; context_switch.asm
; x86-64 таск-свитч в ring 0 long mode
; Структура struct task (см. task.h):
;   0:  r15
;   8:  r14
;   16: r13
;   24: r12
;   32: r11
;   40: r10
;   48: r9
;   56: r8
;   64: rdi
;   72: rsi
;   80: rbp
;   88: rbx
;   96: rdx
;   104: rcx
;   112: rax
;   120: rip
;   128: rflags
;   136: rsp
;   144: ss
;   152: cs

section .text

global save_current_context
global switch_to_task
global test_task_entry

; void save_current_context(struct task* current);
save_current_context:
    ; Сохраняем RSP до push
    mov [rdi + 136], rsp

    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; rdi всё ещё указывает на структуру — используем его
    mov [rdi + 0],   r15
    mov [rdi + 8],   r14
    mov [rdi + 16],  r13
    mov [rdi + 24],  r12
    mov [rdi + 32],  r11
    mov [rdi + 40],  r10
    mov [rdi + 48],  r9
    mov [rdi + 56],  r8
    mov [rdi + 64],  rdi
    mov [rdi + 72],  rsi
    mov [rdi + 80],  rbp
    mov [rdi + 88],  rbx
    mov [rdi + 96],  rdx
    mov [rdi + 104], rcx
    mov [rdi + 112], rax

    pushfq
    pop rax
    mov [rdi + 128], rax        ; rflags

    lea rax, [.return]
    mov [rdi + 120], rax        ; rip

    mov qword [rdi + 144], 0x10 ; ss
    mov qword [rdi + 152], 0x08 ; cs

.return:
    add rsp, 15*8
    ret

; void switch_to_task(struct task* next);
switch_to_task:
    ; Используем callee-saved регистр r15 для хранения указателя
    mov r15, rdi                ; r15 = указатель на struct task

    ; Загружаем все регистры, КРОМЕ rdi — в обратном порядке
    mov r14, [r15 + 8]
    mov r13, [r15 + 16]
    mov r12, [r15 + 24]
    mov r11, [r15 + 32]
    mov r10, [r15 + 40]
    mov r9,  [r15 + 48]
    mov r8,  [r15 + 56]
    mov rsi, [r15 + 72]
    mov rbp, [r15 + 80]
    mov rbx, [r15 + 88]
    mov rdx, [r15 + 96]
    mov rcx, [r15 + 104]
    mov rax, [r15 + 112]

    ; rdi — последним
    mov rdi, [r15 + 64]

    ; === ОТЛАДКА: вывод 'S' в serial ===
    mov dx, 0x3F8
    mov al, 'S'
    out dx, al

    ; Формируем фрейм для iretq
    push qword [r15 + 144]  ; ss
    push qword [r15 + 136]  ; rsp
    push qword [r15 + 128]  ; rflags
    push qword [r15 + 152]  ; cs
    push qword [r15 + 120]  ; rip

    iretq

; Точка входа для тестовой задачи
test_task_entry:
    mov dx, 0x3F8
    mov al, 'X'
    out dx, al
.loop:
    hlt
    jmp .loop