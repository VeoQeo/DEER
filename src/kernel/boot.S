# === MB2.S: Multiboot2 Header ===
.set	MAGIC, 0xE85250D6		# Multiboot2 magic number
.set	ARCH, 0				# Architecture: i386 (x86_64 совместим)
# CHECKSUM вычисляется автоматически

.section .multiboot
.align 8

mbh_start:
	.long	MAGIC
	.long	ARCH
	.long	mbh_end - mbh_start	# Header length
	.long	-(MAGIC + ARCH + (mbh_end - mbh_start))  # CHECKSUM

	# Framebuffer request tag (type=5)
	.word	5					# type
	.word	20					# size
	.long	800					# width
	.long	600					# height
	.long	32					# depth (bits per pixel)

	# End tag
	.word	0
	.word	0
	.long	8
mbh_end:


# === rodata: GDT и таблицы страниц ===
.section .rodata
.align 8

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
    movl    $stack_top, %esp
    # %eax = magic, %ebx = mb2_info_ptr

    call    check_multiboot
    call    check_cpuid
    call    check_long_mode
    call    set_up_page_tables
    call    enable_paging
    call    set_up_SSE

    lgdt    gdt64_pointer

    movw    $gdt64_data_seg, %ax
    movw    %ax, %ss
    movw    %ax, %ds
    movw    %ax, %es

    # Переход в 64-битный режим
    ljmp    $gdt64_code_seg, $kernel_main64_entry


# === Точка входа в 64-битный код ===
.code64
kernel_main64_entry:
    # Очистка сегментных регистров
    movw    $0, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %ss

    # Передача аргументов в соответствии с System V ABI:
    # rdi = magic, rsi = mb2_info_ptr
    movl    %eax, %edi        # magic → rdi
    movl    %ebx, %esi        # mb2_info_ptr → rsi

    call    kernel_main

    # Зацикливаемся
    hlt
    jmp     .


# === Вспомогательные функции (32-bit) ===
.code32

set_up_page_tables:
    # Настройка 4 уровня пейджинга (P4 -> P3 -> P2)
    movl    $p4_table, %eax
    orl     $0b11, %eax              # present + writable
    movl    %eax, p4_table + 511 * 8 # P4[511] = адрес P4 (рекурсивная запись)

    movl    $p3_table, %eax
    orl     $0b11, %eax
    movl    %eax, p4_table           # P4[0] = P3

    movl    $p2_table, %eax
    orl     $0b11, %eax
    movl    %eax, p3_table           # P3[0] = P2

    # Маппим 2048 страниц по 2MB = 4 ГБ
    movl    $0, %ecx
map_p2_table:
    movl    $0x200000, %eax          # 2MB
    mull    %ecx                     # eax = ecx * 0x200000
    orl     $0b10000011, %eax        # present + writable + huge page
    movl    %eax, p2_table(,%ecx,8)  # P2[ecx] = entry
    incl    %ecx
    cmpl    $2048, %ecx              # 2048 * 2MB = 4 GB
    jne     map_p2_table

    ret


enable_paging:
    movl    $p4_table, %eax
    movl    %eax, %cr3               # CR3 = физический адрес P4

    movl    %cr4, %eax
    orl     $1 << 5, %eax            # Включаем PAE
    movl    %eax, %cr4

    movl    $0xC0000080, %ecx        # EFER MSR
    rdmsr
    orl     $1 << 8, %eax            # Устанавливаем LME (Long Mode Enable)
    wrmsr

    movl    %cr0, %eax
    orl     $1 << 31, %eax           # Включаем paging (PG bit)
    movl    %eax, %cr0

    ret


set_up_SSE:
    movl    $0x1, %eax
    cpuid
    testl   $1<<25, %edx             # Поддерживает ли SSE?
    jz      no_SSE

    movl    %cr0, %eax
    andl    $0xFFFB, %eax            # Clear EM (CR0.EM)
    orl     $0x2, %eax               # Set MP (CR0.MP)
    movl    %eax, %cr0

    movl    %cr4, %eax
    orl     $3 << 9, %eax            # Set OSFXSR and OSXMMEXCPT
    movl    %eax, %cr4

    ret


# === Проверки запуска ===

check_multiboot:
    cmpl    $0x36d76289, %eax
    jne     no_multiboot
    ret

no_multiboot:
    movb    $'0', %al
    jmp     error

check_cpuid:
    pushfl
    popl    %eax
    movl    %eax, %ecx
    xorl    $1 << 21, %eax
    pushl   %eax
    popfl
    pushfl
    popl    %eax
    pushl   %ecx
    popfl
    xorl    %ecx, %eax
    jz      no_cpuid
    ret

no_cpuid:
    movb    $'1', %al
    jmp     error

check_long_mode:
    movl    $0x80000000, %eax
    cpuid
    cmpl    $0x80000001, %eax
    jb      no_long_mode
    movl    $0x80000001, %eax
    cpuid
    testl   $1 << 29, %edx
    jz      no_long_mode
    ret

no_long_mode:
    movb    $'2', %al
    jmp     error

no_SSE:
    movb    $'a', %al
    jmp     error

error:
    movl    $0x4f524f45, 0xb8000     # "ERRO"
    movl    $0x4f3a4f52, 0xb8004     # "RO:"
    movl    $0x4f204f20, 0xb8008     # "  "
    movb    %al, 0xb800a             # Код ошибки
    hlt


# === Секция BSS ===
.section .bss
.align 4096

p4_table:
    .skip 4096
p3_table:
    .skip 4096
p2_table:
    .skip 4096 * 4   # 16 KB для 2048 записей

stack_bottom:
    .skip 4096 * 2
stack_top: