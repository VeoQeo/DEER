; context_switch.asm
; x86-64 task switch (ring 0, long mode) — CORRECTED
; Layout (offsets in struct task):
;   0: r15, 8: r14, 16: r13, 24: r12, 32: r11, 40: r10,
;   48: r9, 56: r8, 64: rdi, 72: rsi, 80: rbp, 88: rbx,
;   96: rdx, 104: rcx, 112: rax,
;   120: rip, 128: rflags, 136: rsp, 144: ss, 152: cs

section .text

global save_current_context
global switch_to_task

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

    ; Сохраняем в структуру (rdi не менялся)
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
    ; Сохраняем адрес структуры — он нам понадобится до конца!
    ; Не используем rdi после загрузки из [rax+64]!
    mov r10, rdi                ; r10 = указатель на struct task

    ; Загружаем регистры, КРОМЕ rdi
    mov r15, [r10 + 0]
    mov r14, [r10 + 8]
    mov r13, [r10 + 16]
    mov r12, [r10 + 24]
    mov r11, [r10 + 32]
    mov r10, [r10 + 40]         ; временно: r10 = сохранённый r10 (но мы его не используем)
    ; Вместо этого — перезагрузим указатель!
    mov r10, rdi                ; возвращаем указатель!

    mov r9,  [r10 + 48]
    mov r8,  [r10 + 56]
    ; Теперь загружаем rdi ПОСЛЕДНИМ
    mov rsi, [r10 + 72]         ; сохраняем rsi как временный
    mov rdi, [r10 + 64]         ; теперь rdi = сохранённый rdi

    mov rbp, [r10 + 80]
    mov rbx, [r10 + 88]
    mov rdx, [r10 + 96]
    mov rcx, [r10 + 104]
    mov rax, [r10 + 112]

    ; === ОТЛАДКА: вывод 'S' в serial port (0x3F8) ===
    mov dx, 0x3F8
    mov al, 'S'
    out dx, al

    ; Формируем IRET-фрейм НА ТЕКУЩЕМ СТЕКЕ (ядра)
    ; Важно: используем ИСХОДНЫЙ УКАЗАТЕЛЬ r10!
    push qword [r10 + 144]  ; ss
    push qword [r10 + 136]  ; rsp
    push qword [r10 + 128]  ; rflags
    push qword [r10 + 152]  ; cs
    push qword [r10 + 120]  ; rip

    ; === ОТЛАДКА: проверка, что rip != 0 ===
    ; (необязательно, но можно добавить)

    iretq

; === Точка входа для тестовой задачи (для отладки) ===
; Эту функцию можно вызвать из C как extern void test_task_entry(void);
global test_task_entry
test_task_entry:
    ; Вывод 'X' — задача запущена!
    mov dx, 0x3F8
    mov al, 'X'
    out dx, al

.loop:
    hlt
    jmp .loop