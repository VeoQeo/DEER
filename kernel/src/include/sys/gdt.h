#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// Структура дескриптора GDT
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

// Структура указателя GDT (ИСПРАВЛЕНО - должен быть 48-битным)
struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

// Сегментные селекторы (ИСПРАВЛЕНО - учитываем смещение)
#define GDT_KERNEL_CS 0x08  // 1-й дескриптор (после null)
#define GDT_KERNEL_DS 0x10  // 2-й дескриптор

// Флаги доступа
#define GDT_ACCESS_PRESENT     (1 << 7)
#define GDT_ACCESS_RING0       (0 << 5)
#define GDT_ACCESS_SEGMENT     (1 << 4)
#define GDT_ACCESS_EXECUTABLE  (1 << 3)
#define GDT_ACCESS_READ_WRITE  (1 << 1)

// Флаги гранулярности
#define GDT_GRANULARITY_4K     (1 << 7)
#define GDT_GRANULARITY_LONG   (1 << 5)  // 64-bit mode

// Функции
void gdt_init(void);
void gdt_load(void);

// Функция для получения дескриптора GDT (для отладки)
const struct gdt_entry* gdt_get_descriptor(int index);



#endif // GDT_H