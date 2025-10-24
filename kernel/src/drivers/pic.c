#include "include/drivers/pic.h"
#include "include/drivers/io.h"
#include "include/drivers/serial.h"

void pic_remap(void) {
    serial_puts("[PIC] Remapping PIC...\n");
    
    // Сохраняем маски
    uint8_t pic1_mask = inb(PIC1_DATA);
    uint8_t pic2_mask = inb(PIC2_DATA);
    
    // ICW1 - инициализация
    outb(PIC1_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    outb(PIC2_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    
    // ICW2 - смещения
    outb(PIC1_DATA, PIC1_OFFSET);   // IRQ 0-7 -> INT 32-39
    outb(PIC2_DATA, PIC2_OFFSET);   // IRQ 8-15 -> INT 40-47
    
    // ICW3 - каскадирование
    outb(PIC1_DATA, 0x04);          // PIC1 имеет slave на IRQ2
    outb(PIC2_DATA, 0x02);          // PIC2 cascade identity
    
    // ICW4 - дополнительная инициализация
    outb(PIC1_DATA, PIC_ICW4_8086);
    outb(PIC2_DATA, PIC_ICW4_8086);
    
    // Восстанавливаем маски
    outb(PIC1_DATA, pic1_mask);
    outb(PIC2_DATA, pic2_mask);
    
    serial_puts("[PIC] PIC remapped to 0x20-0x2F\n");
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_disable(void) {
    // Маскируем все IRQ
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
    serial_puts("[PIC] PIC disabled\n");
}

void pic_mask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) | (1 << irq);
    outb(port, value);
}

void pic_unmask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}