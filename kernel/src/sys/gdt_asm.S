.section .text
.global gdt_flush

gdt_flush:
    // rdi содержит указатель на struct gdt_ptr
    lgdt (%rdi)
    
    // Обновляем сегментные регистры
    mov $0x10, %ax    // Kernel Data Segment selector (8-й дескриптор * 8 = 0x10)
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    
    // Делаем дальний переход для обновления CS
    pushq $0x08       // Kernel Code Segment selector (7-й дескриптор * 8 = 0x08)
    lea .flush_here(%rip), %rax
    pushq %rax
    lretq

.flush_here:
    ret