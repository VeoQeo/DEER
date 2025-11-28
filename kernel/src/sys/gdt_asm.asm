; Переписан с GAS на NASM

section .text
global gdt_flush

gdt_flush:
    ; rdi содержит указатель на struct gdt_ptr
    lgdt [rdi]

    ; Обновляем сегментные регистры
    mov ax, 0x10          ; Kernel Data Segment selector (8-й дескриптор * 8 = 0x10)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Делаем дальний переход для обновления CS
    push 0x08             ; Kernel Code Segment selector (7-й дескриптор * 8 = 0x08)
    lea rax, [.flush_here]
    push rax
    retfq

.flush_here:
    ret