# === MB2.S: Multiboot2 Header ===
.set	MAGIC, 0xE85250D6		# Multiboot2 magic number
.set	ARCH, 0				# Architecture: i386 (x86_64 совместим)
.set	CHECKSUM, -(MAGIC + ARCH + (mbh_end - mbh_start))

.section .multiboot
.align 4

mbh_start:
	.long	MAGIC
	.long	ARCH
	.long	mbh_end - mbh_start	# Header length
	.long	CHECKSUM

	# End tag
	.word	0					# type: end tag
	.word	0					# flags
	.long	8					# size = 8 bytes (only header)
mbh_end:


# === boot.s: 32-bit Bootloader to Long Mode ===
.section .rodata
.align 4

gdt64:
	.quad	0						# Null segment

gdt64_code_entry:
	.set	gdt64_code_seg, gdt64_code_entry - gdt64
	.quad	(1<<44) | (1<<47) | (1<<41) | (1<<43) | (1<<53)  # Code segment (64-bit)

gdt64_data_entry:
	.set	gdt64_data_seg, gdt64_data_entry - gdt64
	.quad	(1<<44) | (1<<47) | (1<<41)                      # Data segment

gdt64_pointer:
	.set	gdt64_limit, gdt64_pointer - gdt64 - 1
	.word	gdt64_limit
	.quad	gdt64


# === Точка входа ===
.global _start
.extern kernel_main

.section .text
.code32
_start:
	movl	$stack_top, %esp

	movl	%ebx, %edi			# Передаём multiboot info struct в %edi

	# Проверки перед переходом
	call	check_multiboot
	call	check_cpuid
	call	check_long_mode
	call	set_up_page_tables
	call	enable_paging
	call	set_up_SSE

	# Загружаем GDT
	lgdt	(gdt64_pointer)

	# Устанавливаем data segments на data selector
	movw	$gdt64_data_seg, %ax
	movw	%ax, %ss
	movw	%ax, %ds
	movw	%ax, %es

	# Переход в 64-битный режим
	ljmp	$gdt64_code_seg, $kernel_main


# === Вспомогательные функции ===

set_up_page_tables:
	# Настройка 4 уровня пейджинга (P4 -> P3 -> P2)
	mov	$p4_table, %eax
	orl	$0b11, %eax				# present + writable
	movl	%eax, (p4_table + 511 * 8)	# P4[511] = адрес P4 (рекурсивная запись)

	movl	$p3_table, %eax
	orl	$0b11, %eax
	movl	%eax, (p4_table)		# P4[0] = P3

	movl	$p2_table, %eax
	orl	$0b11, %eax
	mov	%eax, (p3_table)		# P3[0] = P2

	# Маппим 512 страниц по 2MB каждая (1GB) в P2
	movl	$0, %ecx
map_p2_table:
	movl	$0x200000, %eax		# 2MB base per entry
	mul	%ecx
	orl	$0b10000011, %eax		# present + writable + huge page
	movl	%eax, p2_table(,%ecx,8)	# P2[ecx] = entry
	inc	%ecx
	cmp	$512, %ecx
	jne	map_p2_table

	ret


enable_paging:
	movl	$p4_table, %eax
	movl	%eax, %cr3			# CR3 = физический адрес P4

	movl	%cr4, %eax
	orl	$1 << 5, %eax			# Включаем PAE
	mov	%eax, %cr4

	mov	$0xC0000080, %ecx		# EFER MSR
	rdmsr
	orl	$1 << 8, %eax			# Устанавливаем LME (Long Mode Enable)
	wrmsr

	movl	%cr0, %eax
	orl	$1 << 31, %eax			# Включаем paging (PG bit)
	mov	%eax, %cr0

	ret


set_up_SSE:
	movl	$0x1, %eax
	cpuid
	testl	$1<<25, %edx			# Поддерживает ли SSE?
	jz	no_SSE

	movl	%cr0, %eax
	andw	$0xFFFB, %ax			# Clear EM (CR0.EM)
	orw	$0x2, %ax				# Set MP (CR0.MP)
	movl	%eax, %cr0

	movl	%cr4, %eax
	orw	$3 << 9, %ax			# Set OSFXSR and OSXMMEXCPT (SSE разрешён)
	movl	%eax, %cr4

	ret


# === Проверки запуска ===

check_multiboot:
	cmpl	$0x36d76289, %eax
	jne	no_multiboot
	ret

no_multiboot:
	movb	$'0', %al
	jmp	error

check_cpuid:
	pushf
	pop	%eax
	mov	%eax, %ecx
	xor	$1 << 21, %eax
	push	%eax
	popf
	pushf
	pop	%eax
	push	%ecx
	popf
	xor	%ecx, %eax
	jz	no_cpuid
	ret

no_cpuid:
	mov	$'1', %al
	jmp	error

check_long_mode:
	movl	$0x80000000, %eax
	cpuid
	cmp	$0x80000001, %eax
	jb	no_long_mode
	movl	$0x80000001, %eax
	cpuid
	test	$1 << 29, %edx
	jz	no_long_mode
	ret

no_long_mode:
	mov	$'2', %al
	jmp	error

no_SSE:
	movb	$'a', %al
	jmp	error


error:
	movl	$0x4f524f45, (0xb8000)   # "ERRO"
	movl	$0x4f3a4f52, (0xb8004)   # "RO:"
	movl	$0x4f204f20, (0xb8008)   # "  "
	movb	%al, (0xb800a)           # Код ошибки
	hlt


# === Секция BSS (неинициализированные данные) ===
.section .bss
.align 4096

p4_table:
	.skip 4096
p3_table:
	.skip 4096
p2_table:
	.skip 4096

stack_bottom:
	.skip 4096 * 2
stack_top: