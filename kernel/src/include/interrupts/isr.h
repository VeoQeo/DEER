#ifndef ISR_H
#define ISR_H

#include <stdint.h>

// Структура регистров при прерывании
struct registers {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed));

// Тип обработчика прерываний
typedef void (*isr_handler_t)(struct registers*);

// Функции
void isr_install_handler(uint8_t num, isr_handler_t handler);
void isr_uninstall_handler(uint8_t num);
void exception_handler(struct registers *regs);
void irq_handler(struct registers *regs);

#endif // ISR_H