.section .text

// Макрос для создания заглушки без кода ошибки
.macro ISR_NOERRCODE index
    .global isr_stub_\index
    .type isr_stub_\index, @function
isr_stub_\index:
    push $0                    // Пустой код ошибки
    push $\index               // Номер прерывания
    jmp isr_common_stub
.endm

// Макрос для создания заглушки с кодом ошибки
.macro ISR_ERRCODE index
    .global isr_stub_\index
    .type isr_stub_\index, @function
isr_stub_\index:
    push $\index               // Номер прерывания (код ошибки уже в стеке)
    jmp isr_common_stub
.endm

// Создаем заглушки для исключений
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

// Создаем заглушки для IRQ (32-47)
ISR_NOERRCODE 32
ISR_NOERRCODE 33
ISR_NOERRCODE 34
ISR_NOERRCODE 35
ISR_NOERRCODE 36
ISR_NOERRCODE 37
ISR_NOERRCODE 38
ISR_NOERRCODE 39
ISR_NOERRCODE 40
ISR_NOERRCODE 41
ISR_NOERRCODE 42
ISR_NOERRCODE 43
ISR_NOERRCODE 44
ISR_NOERRCODE 45
ISR_NOERRCODE 46
ISR_NOERRCODE 47

// Общий обработчик для всех прерываний
.extern isr_handler
.type isr_common_stub, @function
isr_common_stub:
    // Сохраняем все регистры
    push %r15
    push %r14
    push %r13
    push %r12
    push %r11
    push %r10
    push %r9
    push %r8
    push %rdi
    push %rsi
    push %rbp
    push %rbx
    push %rdx
    push %rcx
    push %rax
    
    // Вызываем C-обработчик
    mov %rsp, %rdi
    call isr_handler
    
    // Восстанавливаем регистры
    pop %rax
    pop %rcx
    pop %rdx
    pop %rbx
    pop %rbp
    pop %rsi
    pop %rdi
    pop %r8
    pop %r9
    pop %r10
    pop %r11
    pop %r12
    pop %r13
    pop %r14
    pop %r15
    
    // Очищаем номер прерывания и код ошибки
    add $16, %rsp
    
    iretq

// Загрузка IDT
.global idt_flush
.type idt_flush, @function
idt_flush:
    lidt (%rdi)
    ret