#include "include/interrupts/isr.h"
#include "include/interrupts/idt.h"  // <-- ДОБАВЬ ЭТУ СТРОКУ
#include "include/drivers/serial.h"
#include "libc/string.h"

// Сообщения об исключениях
static const char *exception_messages[32] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

void exception_handler(struct registers *regs) {
    char buffer[128];
    
    serial_puts("\n[EXCEPTION] ");
    serial_puts(exception_messages[regs->int_no]);
    serial_puts("\n");
    
    serial_puts("Error Code: 0x");
    serial_puts(itoa(regs->err_code, buffer, 16));
    serial_puts("\n");
    
    serial_puts("RIP: 0x");
    serial_puts(itoa(regs->rip, buffer, 16));
    serial_puts("\n");
    
    serial_puts("CS: 0x");
    serial_puts(itoa(regs->cs, buffer, 16));
    serial_puts("\n");
    
    serial_puts("RFLAGS: 0x");
    serial_puts(itoa(regs->rflags, buffer, 16));
    serial_puts("\n");
    
    serial_puts("RSP: 0x");
    serial_puts(itoa(regs->rsp, buffer, 16));
    serial_puts("\n");
    
    // Для Page Fault выводим дополнительную информацию
    if (regs->int_no == EXCEPTION_PAGE_FAULT) {
        uint64_t cr2;
        asm volatile("mov %%cr2, %0" : "=r"(cr2));
        serial_puts("CR2: 0x");
        serial_puts(itoa(cr2, buffer, 16));
        serial_puts("\n");
    }
    
    // Зависаем при исключении
    serial_puts("[SYSTEM HALTED]\n");
    for(;;) {
        asm volatile("hlt");
    }
}
