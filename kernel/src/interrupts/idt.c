#include "include/interrupts/idt.h"
#include "include/interrupts/isr.h"
#include "include/drivers/serial.h"
#include "include/memory/paging.h"
#include "libc/string.h" 
#include <stddef.h>

static struct idt_entry idt[256];
struct idt_ptr idtp;

extern void idt_flush(uint64_t idt_ptr);

extern void isr_stub_0(void);
extern void isr_stub_1(void);
extern void isr_stub_2(void);
extern void isr_stub_3(void);
extern void isr_stub_4(void);
extern void isr_stub_5(void);
extern void isr_stub_6(void);
extern void isr_stub_7(void);
extern void isr_stub_8(void);
extern void isr_stub_9(void);
extern void isr_stub_10(void);
extern void isr_stub_11(void);
extern void isr_stub_12(void);
extern void isr_stub_13(void);
extern void isr_stub_14(void);
extern void isr_stub_15(void);
extern void isr_stub_16(void);
extern void isr_stub_17(void);
extern void isr_stub_18(void);
extern void isr_stub_19(void);
extern void isr_stub_20(void);
extern void isr_stub_21(void);
extern void isr_stub_22(void);
extern void isr_stub_23(void);
extern void isr_stub_24(void);
extern void isr_stub_25(void);
extern void isr_stub_26(void);
extern void isr_stub_27(void);
extern void isr_stub_28(void);
extern void isr_stub_29(void);
extern void isr_stub_30(void);
extern void isr_stub_31(void);

extern void isr_stub_32(void);
extern void isr_stub_33(void);
extern void isr_stub_34(void);
extern void isr_stub_35(void);
extern void isr_stub_36(void);
extern void isr_stub_37(void);
extern void isr_stub_38(void);
extern void isr_stub_39(void);
extern void isr_stub_40(void);
extern void isr_stub_41(void);
extern void isr_stub_42(void);
extern void isr_stub_43(void);
extern void isr_stub_44(void);
extern void isr_stub_45(void);
extern void isr_stub_46(void);
extern void isr_stub_47(void);

static isr_handler_t isr_handlers[256] = {0};

// Декларация обработчика из isr.c
extern void isr_handler(struct registers *regs);

void idt_set_entry(uint8_t index, uint64_t base, uint16_t selector, uint8_t type_attr) {
    idt[index].base_low = base & 0xFFFF;
    idt[index].base_middle = (base >> 16) & 0xFFFF;
    idt[index].base_high = (base >> 32) & 0xFFFFFFFF;
    idt[index].selector = selector;
    idt[index].ist = 0;
    idt[index].type_attr = type_attr;
    idt[index].zero = 0;
}

void idt_init(void) {

    serial_puts("[IDT] Initializing IDT...\n");
    
    // Обнуляем IDT
    for (int i = 0; i < 256; i++) {
        idt_set_entry(i, 0, 0, 0);
    }
    
    // Устанавливаем обработчики исключений (0-31)
    idt_set_entry(EXCEPTION_DIVISION_ERROR, (uint64_t)isr_stub_0, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(EXCEPTION_DEBUG, (uint64_t)isr_stub_1, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(EXCEPTION_NMI, (uint64_t)isr_stub_2, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(EXCEPTION_BREAKPOINT, (uint64_t)isr_stub_3, 0x08, IDT_PRESENT | IDT_RING3 | IDT_TYPE_INT);
    idt_set_entry(EXCEPTION_OVERFLOW, (uint64_t)isr_stub_4, 0x08, IDT_PRESENT | IDT_RING3 | IDT_TYPE_INT);
    idt_set_entry(EXCEPTION_BOUND_RANGE, (uint64_t)isr_stub_5, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(EXCEPTION_INVALID_OPCODE, (uint64_t)isr_stub_6, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(EXCEPTION_DEVICE_NOT_AVAILABLE, (uint64_t)isr_stub_7, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(EXCEPTION_DOUBLE_FAULT, (uint64_t)isr_stub_8, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(EXCEPTION_COPROCESSOR_SEGMENT, (uint64_t)isr_stub_9, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(EXCEPTION_INVALID_TSS, (uint64_t)isr_stub_10, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(EXCEPTION_SEGMENT_NOT_PRESENT, (uint64_t)isr_stub_11, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(EXCEPTION_STACK_SEGMENT_FAULT, (uint64_t)isr_stub_12, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(EXCEPTION_GPF, (uint64_t)isr_stub_13, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(EXCEPTION_PAGE_FAULT, (uint64_t)isr_stub_14, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(EXCEPTION_RESERVED, (uint64_t)isr_stub_15, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(EXCEPTION_X87_FPU, (uint64_t)isr_stub_16, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(EXCEPTION_ALIGNMENT_CHECK, (uint64_t)isr_stub_17, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(EXCEPTION_MACHINE_CHECK, (uint64_t)isr_stub_18, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(EXCEPTION_SIMD_FPU, (uint64_t)isr_stub_19, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(EXCEPTION_VIRTUALIZATION, (uint64_t)isr_stub_20, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(21, (uint64_t)isr_stub_21, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(22, (uint64_t)isr_stub_22, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(23, (uint64_t)isr_stub_23, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(24, (uint64_t)isr_stub_24, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(25, (uint64_t)isr_stub_25, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(26, (uint64_t)isr_stub_26, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(27, (uint64_t)isr_stub_27, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(28, (uint64_t)isr_stub_28, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(29, (uint64_t)isr_stub_29, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(EXCEPTION_SECURITY, (uint64_t)isr_stub_30, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(31, (uint64_t)isr_stub_31, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    
    idt_set_entry(32, (uint64_t)isr_stub_32, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(33, (uint64_t)isr_stub_33, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(34, (uint64_t)isr_stub_34, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(35, (uint64_t)isr_stub_35, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(36, (uint64_t)isr_stub_36, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(37, (uint64_t)isr_stub_37, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(38, (uint64_t)isr_stub_38, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(39, (uint64_t)isr_stub_39, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(40, (uint64_t)isr_stub_40, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(41, (uint64_t)isr_stub_41, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(42, (uint64_t)isr_stub_42, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(43, (uint64_t)isr_stub_43, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(44, (uint64_t)isr_stub_44, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(45, (uint64_t)isr_stub_45, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(46, (uint64_t)isr_stub_46, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    idt_set_entry(47, (uint64_t)isr_stub_47, 0x08, IDT_PRESENT | IDT_RING0 | IDT_TYPE_INT);
    // Настраиваем указатель IDT
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint64_t)&idt;
    
    // Устанавливаем обработчики исключений
    for (int i = 0; i < 32; i++) {
        isr_install_handler(i, exception_handler);
    };

    isr_install_handler(EXCEPTION_PAGE_FAULT, handle_page_fault);
    isr_install_handler(EXCEPTION_GPF, handle_general_protection_fault);
    isr_install_handler(EXCEPTION_DOUBLE_FAULT, handle_double_fault);
    
    serial_puts("[IDT] IDT entries set up (0-31: exceptions)\n");
}

void idt_load(void) {
    serial_puts("[IDT] Loading IDT...\n");
    idt_flush((uint64_t)&idtp);
    asm volatile("sti");  // Разрешаем прерывания
    serial_puts("[IDT] Interrupts enabled\n");
}

void isr_install_handler(uint8_t num, isr_handler_t handler) {
    isr_handlers[num] = handler;
    
    char buffer[16];
    serial_puts("[IDT] Installed handler for interrupt ");
    serial_puts(itoa(num, buffer, 10));
    serial_puts("\n");
}

void isr_uninstall_handler(uint8_t num) {
    isr_handlers[num] = NULL;
}

void isr_handler(struct registers *regs) {
    if (isr_handlers[regs->int_no] != NULL) {
        isr_handlers[regs->int_no](regs);
    } else {
        if (regs->int_no >= 32 && regs->int_no < 48) {
            irq_handler(regs);
        } else {
            char buffer[64];
            serial_puts("[ISR] No handler for interrupt ");
            serial_puts(itoa(regs->int_no, buffer, 10));
            serial_puts("\n");
            
            if (regs->int_no < 32) {
                serial_puts("[ISR] Unhandled exception! System halted.\n");
                for(;;) {
                    asm volatile("hlt");
                }
            }
        }
    }
}