#include "include/interrupts/irq.h"
#include "include/drivers/pic.h"
#include "include/interrupts/idt.h"
#include "include/drivers/serial.h"
#include "libc/string.h"

static isr_handler_t irq_handlers[16] = {0};

// Обработчик по умолчанию для IRQ (объявляем ПЕРВЫМ)
void irq_default_handler(struct registers *regs) {
    char buffer[64];
    serial_puts("[IRQ] Default handler for IRQ");
    serial_puts(itoa(regs->int_no - 32, buffer, 10));
    serial_puts("\n");
}

// Главный обработчик IRQ
void irq_handler(struct registers *regs) {
    uint8_t irq_num = regs->int_no - 32;
    
    // Вызываем зарегистрированный обработчик
    if (irq_handlers[irq_num] != NULL) {
        irq_handlers[irq_num](regs);
    } else {
        irq_default_handler(regs);
    }
    
    // Отправляем EOI в PIC
    pic_send_eoi(irq_num);
}

void irq_init(void) {
    serial_puts("[IRQ] Initializing IRQ handlers...\n");
    
    // Устанавливаем обработчики по умолчанию
    for (int i = 0; i < 16; i++) {
        irq_install_handler(i, irq_default_handler);
    }
    
    serial_puts("[IRQ] IRQ handlers initialized\n");
}

void irq_install_handler(uint8_t irq, isr_handler_t handler) {
    if (irq < 16) {
        irq_handlers[irq] = handler;
        
        char buffer[16];
        serial_puts("[IRQ] Installed handler for IRQ");
        serial_puts(itoa(irq, buffer, 10));
        serial_puts("\n");
        
        // Размаскируем IRQ
        pic_unmask_irq(irq);
    }
}

void irq_uninstall_handler(uint8_t irq) {
    if (irq < 16) {
        irq_handlers[irq] = NULL;
        // Маскируем IRQ
        pic_mask_irq(irq);
    }
}