#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <limine.h>
#include "include/graphics/fb.h"
#include "include/graphics/color.h"
#include "include/graphics/font.h"
#include "include/drivers/serial.h"
#include "include/graphics/banner.h"
#include "include/drivers/io.h"
#include "include/sys/gdt.h"
#include "libc/string.h"
#include "libc/stdio.h"
#include "include/interrupts/idt.h"
#include "include/interrupts/isr.h"
#include "include/drivers/pic.h"
#include "include/drivers/pit.h"
#include "include/interrupts/irq.h"

// Limine requests...
__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

// Тестовые функции
void test_gdt_registers(void) {
    uint16_t cs, ds, es, fs, gs, ss;
    
    asm volatile("mov %%cs, %0" : "=r"(cs));
    asm volatile("mov %%ds, %0" : "=r"(ds));
    asm volatile("mov %%es, %0" : "=r"(es));
    asm volatile("mov %%fs, %0" : "=r"(fs));
    asm volatile("mov %%gs, %0" : "=r"(gs));
    asm volatile("mov %%ss, %0" : "=r"(ss));
    
    serial_puts("[GDT-TEST] Segment registers:\n");
    
    char buffer[64];
    serial_puts("CS: 0x");
    serial_puts(itoa(cs, buffer, 16));
    serial_puts(" (should be 0x08)\n");
    
    serial_puts("DS: 0x");
    serial_puts(itoa(ds, buffer, 16));
    serial_puts(" (should be 0x10)\n");
    
    serial_puts("ES: 0x");
    serial_puts(itoa(es, buffer, 16));
    serial_puts(" (should be 0x10)\n");
    
    serial_puts("FS: 0x");
    serial_puts(itoa(fs, buffer, 16));
    serial_puts(" (should be 0x10)\n");
    
    serial_puts("GS: 0x");
    serial_puts(itoa(gs, buffer, 16));
    serial_puts(" (should be 0x10)\n");
    
    serial_puts("SS: 0x");
    serial_puts(itoa(ss, buffer, 16));
    serial_puts(" (should be 0x10)\n");
}

void test_memory_access(void) {
    serial_puts("[GDT-TEST] Testing memory access...\n");
    
    volatile uint32_t test_value = 0xDEADBEEF;
    volatile uint32_t read_value = 0;
    
    // Простой тест записи/чтения
    read_value = test_value;
    
    char buffer[64];
    serial_puts("Write/read test: wrote 0x");
    serial_puts(itoa(test_value, buffer, 16));
    serial_puts(", read 0x");
    serial_puts(itoa(read_value, buffer, 16));
    serial_puts("\n");
    
    if (test_value == read_value) {
        serial_puts("[GDT-TEST] Memory access: PASS\n");
    } else {
        serial_puts("[GDT-TEST] Memory access: FAIL\n");
    }
}

void dump_gdt_descriptors(void) {
    serial_puts("[GDT-TEST] Dumping GDT descriptors:\n");
    
    for (int i = 0; i < 3; i++) {
        const struct gdt_entry* desc = gdt_get_descriptor(i);
        if (!desc) {
            serial_puts("Descriptor ");
            char buffer[16];
            serial_puts(itoa(i, buffer, 10));
            serial_puts(": NULL\n");
            continue;
        }
        
        char buffer[128];
        serial_puts("Descriptor ");
        serial_puts(itoa(i, buffer, 10));
        serial_puts(": limit=0x");
        
        uint32_t limit = desc->limit_low | ((desc->granularity & 0xF) << 16);
        serial_puts(itoa(limit, buffer, 16));
        
        serial_puts(", base=0x");
        uint32_t base = desc->base_low | (desc->base_middle << 16) | (desc->base_high << 24);
        serial_puts(itoa(base, buffer, 16));
        
        serial_puts(", access=0x");
        serial_puts(itoa(desc->access, buffer, 16));
        
        serial_puts(", granularity=0x");
        serial_puts(itoa(desc->granularity, buffer, 16));
        serial_puts("\n");
    }
}

void kernel_main(void) {
    serial_init();
    serial_puts("[DEER] Kernel started\n");
    
    // Инициализация GDT
    gdt_init();
    gdt_load();
    
    // Инициализация PIC
    pic_remap();
    
    // Инициализация IDT
    idt_init();
    
    // Инициализация IRQ
    irq_init();
    
    // Инициализация PIT (таймер)
    pit_init();
    
    // Устанавливаем обработчик таймера
    irq_install_handler(0, pit_timer_handler);
    
    // Загружаем IDT и включаем прерывания
    idt_load();
    
    serial_puts("[DEER] All subsystems initialized\n");
    serial_puts("[DEER] Timer should tick every second...\n");
    
    // Инициализация фреймбуфера
    if (framebuffer_request.response) {
        struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
        // Настраиваем printf для работы с фреймбуфером
        printf_set_framebuffer(fb);
        printf_set_color(COLOR_GREEN);
        printf_set_bg_color(COLOR_BLACK);
        printf_clear();
        
        // Теперь можно использовать printf!
        printf("Hello Kernel!\n");
        printf("DEER OS Booted Successfully!\n");
        printf("Version: %s\n", "v0.0.1");
    }
    
    // Основной цикл
    while(1) {
        asm volatile("hlt");
    }
}