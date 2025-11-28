#include "include/interrupts/isr.h"
#include "include/interrupts/idt.h"  
#include "include/drivers/serial.h"
#include "libc/string.h"
#include "libc/stdio.h"

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

void handle_double_fault(struct registers *regs) {
    serial_puts("\n[EXCEPTION] DOUBLE FAULT! System halted.\n");
    // Можно вывести regs->rip, но при double fault он может быть недостоверен
    for (;;);
}

void exception_handler(struct registers *regs) {
    if (regs->int_no < 32) {
        printf("\n=== EXCEPTION %d ===\n", regs->int_no);
        printf("%s\n", exception_messages[regs->int_no]);
        printf("Error Code: 0x%x\n", regs->err_code);
        printf("RIP: 0x%x\n", regs->rip);
        
        if (regs->int_no == EXCEPTION_PAGE_FAULT) {
            uint64_t cr2;
            asm volatile("mov %%cr2, %0" : "=r"(cr2));
            printf("Fault Address: 0x%x\n", cr2);
        }
        
        if (regs->int_no == EXCEPTION_BREAKPOINT || regs->int_no == EXCEPTION_OVERFLOW) {
            printf("Non-critical exception, continuing...\n");
            return;
        }
        
        printf("System Halted.\n");
    }
    
    for(;;) {
        asm volatile("hlt");
    }
}