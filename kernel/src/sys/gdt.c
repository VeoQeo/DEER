#include "include/sys/gdt.h"
#include "include/drivers/serial.h"
#include <stddef.h>

// GDT таблица (8 записей)
static struct gdt_entry gdt[8] = {0};
static struct gdt_ptr gdtp;

// Внешняя функция для загрузки GDT
extern void gdt_flush(uint64_t gdt_ptr);

static void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity) {
    gdt[index].limit_low = limit & 0xFFFF;
    gdt[index].base_low = base & 0xFFFF;
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].access = access;
    gdt[index].granularity = (limit >> 16) & 0x0F;
    gdt[index].granularity |= granularity & 0xF0;
    gdt[index].base_high = (base >> 24) & 0xFF;
}

void gdt_init(void) {
    serial_puts("[GDT] Initializing GDT...\n");
    
    // Null descriptor (обязателен)
    gdt_set_entry(0, 0, 0, 0, 0);
    
    // Kernel Code Segment (index 1)
    gdt_set_entry(1, 0, 0xFFFFF, 
                 GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_SEGMENT | 
                 GDT_ACCESS_EXECUTABLE | GDT_ACCESS_READ_WRITE,
                 GDT_GRANULARITY_4K | GDT_GRANULARITY_LONG | 0xF);
    
    // Kernel Data Segment (index 2)
    gdt_set_entry(2, 0, 0xFFFFF,
                 GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_SEGMENT | 
                 GDT_ACCESS_READ_WRITE,
                 GDT_GRANULARITY_4K | 0xF);
    
    // Остальные дескрипторы оставляем нулевыми для совместимости с Limine
    
    // Настраиваем указатель GDT
    gdtp.limit = sizeof(gdt) - 1;
    gdtp.base = (uint64_t)&gdt;
    
    serial_puts("[GDT] GDT entries set up\n");
}

void gdt_load(void) {
    serial_puts("[GDT] Loading GDT...\n");
    gdt_flush((uint64_t)&gdtp);
}

const struct gdt_entry* gdt_get_descriptor(int index) {
    if (index >= 0 && index < 8) {
        return &gdt[index];
    }
    return NULL;
}