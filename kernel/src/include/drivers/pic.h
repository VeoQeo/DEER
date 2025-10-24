#ifndef PIC_H
#define PIC_H

#include <stdint.h>

// Порты PIC
#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

// Команды PIC
#define PIC_EOI         0x20        // End Of Interrupt

// ICW1
#define PIC_ICW1_INIT   0x10        // Initialization
#define PIC_ICW1_ICW4   0x01        // ICW4 needed

// ICW4
#define PIC_ICW4_8086   0x01        // 8086 mode

// IRQ базовые векторы
#define PIC1_OFFSET     0x20
#define PIC2_OFFSET     0x28

// Функции
void pic_remap(void);
void pic_send_eoi(uint8_t irq);
void pic_disable(void);
void pic_mask_irq(uint8_t irq);
void pic_unmask_irq(uint8_t irq);

#endif // PIC_H