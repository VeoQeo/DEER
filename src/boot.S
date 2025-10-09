
	.section .rodata
	.align	4
gdt64:
	.quad	0 

gdt64_code_entry:
	.set	gdt64_code_seg, gdt64_code_entry - gdt64
	.quad	(1<<44) | (1<<47) | (1<<41) | (1<<43) | (1<<53)
gdt64_data_entry:
	.set	gdt64_data_seg, gdt64_data_entry - gdt64
	.quad	(1<<44) | (1<<47) | (1<<41) 

gdt64_pointer:
	.set	gdt64_limit, gdt64_pointer - gdt64 - 1
	.word	gdt64_limit
	.quad	gdt64

	.global _start
	.extern kernel_main

	.section .text
	.code32
_start:
	movl	$stack_top, %esp

	movl	%ebx, %edi

	call	check_multiboot
	call	check_cpuid
	call	check_long_mode

	call	set_up_page_tables
	call	enable_paging
	call	set_up_SSE

	lgdt	(gdt64_pointer)

	movw	$gdt64_data_seg, %ax
	movw	%ax, %ss
	movw	%ax, %ds
	movw	%ax, %es

	ljmp	$gdt64_code_seg, $kernel_main

set_up_page_tables:
	mov	$p4_table, %eax
	orl	$0b11, %eax		
	movl	%eax, (p4_table + 511 * 8)

	movl	$p3_table, %eax
	orl	$0b11, %eax		
	movl	%eax, (p4_table)

	movl	$p2_table, %eax
	orl	$0b11, %eax		
	mov	%eax, (p3_table)

	movl	$0, %ecx		
map_p2_table:
	movl	$0x200000, %eax		
	mul	%ecx		
	orl	$0b10000011, %eax
	movl	%eax, p2_table(,%ecx,8) 

	inc	%ecx			
	cmp	$512, %ecx		
	jne	map_p2_table		

	ret

enable_paging:
	movl	$p4_table, %eax
	movl	%eax, %cr3

	movl	%cr4, %eax
	orl	$1 << 5, %eax
	mov	%eax, %cr4

	mov	$0xC0000080, %ecx
	rdmsr
	orl	$1 << 8, %eax
	wrmsr

	movl	%cr0, %eax
	orl	$1 << 31, %eax
	mov	%eax, %cr0

	ret

error:
	movl	$0x4f524f45, (0xb8000)
	movl	$0x4f3a4f52, (0xb8004)
	movl	$0x4f204f20, (0xb8008)
	movb	%al, (0xb800a)
	hlt

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

set_up_SSE:
	movl	$0x1, %eax
	cpuid
	testl	$1<<25, %edx
	jz	no_SSE

	movl	%cr0, %eax
	andw	$0xFFFB, %ax		
	orw	$0x2, %ax		
	movl	%eax, %cr0
	movl	%cr4, %eax
	orw	$3 << 9, %ax		
	movl	%eax, %cr4
	ret

no_SSE:
	movb	$'a', %al
	jmp	error

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
