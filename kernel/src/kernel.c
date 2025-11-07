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
#include "include/memory/paging.h"
#include "include/memory/heap.h"
#include "include/memory/pmm.h"
#include "include/memory/security.h"

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_terminal_request terminal_request = {
    .id = LIMINE_TERMINAL_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

static volatile struct limine_hhdm_response *hhdm_response = NULL;
static volatile struct limine_memmap_response *memmap_response = NULL;
static volatile struct limine_terminal_response *terminal_response = NULL;

void kernel_main(void) {
    serial_init();
    serial_puts("[DEER] Kernel started\n");

    hhdm_response = hhdm_request.response;
    memmap_response = memmap_request.response;
    
    if (!hhdm_response) {
        serial_puts("[DEER] ERROR: No HHDM response from bootloader!\n");
    } else {
        serial_puts("[DEER] HHDM available\n");
    }
    
    if (!memmap_response) {
        serial_puts("[DEER] ERROR: No memory map response from bootloader!\n");
    } else {
        serial_puts("[DEER] Memory map available\n");
    }

    serial_puts("[DEER] Initializing GDT...\n");
    gdt_init();
    gdt_load();
    serial_puts("[DEER] GDT initialized\n");

    serial_puts("[DEER] Initializing paging...\n");
    paging_init(hhdm_response);
    serial_puts("[DEER] Paging initialized\n");

    serial_puts("[DEER] Initializing memory(PMM/HEAP)...\n");
    pmm_init(memmap_response, hhdm_response);
    heap_init();
    serial_puts("[DEER] Memory initialized\n");
    serial_puts("[DEER] Initializing memory security...\n");
    memory_security_init();

    // В главном цикле можно добавить периодические проверки
    static uint64_t last_security_check = 0;
    if (pit_get_ticks() - last_security_check > 1000) { // Каждую секунду
        memory_security_check();
        last_security_check = pit_get_ticks();
    }
    serial_puts("[DEER] Initializing PIC...\n");
    pic_remap();
    serial_puts("[DEER] PIC remapped\n");

    serial_puts("[DEER] Initializing IDT...\n");
    idt_init();
    serial_puts("[DEER] IDT initialized\n");

    serial_puts("[DEER] Initializing IRQ...\n");
    irq_init();
    serial_puts("[DEER] IRQ initialized\n");

    serial_puts("[DEER] Initializing PIT...\n");
    pit_init();
    serial_puts("[DEER] PIT initialized\n");

    serial_puts("[DEER] Installing timer handler...\n");
    irq_install_handler(0, pit_timer_handler);
    serial_puts("[DEER] Timer handler installed\n");

    serial_puts("[DEER] Enabling interrupts...\n");
    idt_load();
    serial_puts("[DEER] Interrupts enabled\n");


    serial_puts("[DEER] All subsystems initialized successfully\n");
    serial_puts("[DEER] Timer should tick every second...\n");

    struct limine_framebuffer *fb = NULL;

    if (framebuffer_request.response && framebuffer_request.response->framebuffer_count > 0) {
        // Используем графический режим (существующий код)
        fb = framebuffer_request.response->framebuffers[0];
        printf_init_with_framebuffer(fb);
        printf_set_color(COLOR_GREEN);
        printf_set_bg_color(COLOR_BLACK);
            
        printf_clear();
        printf_set_color(COLOR_ORANGE);
        printf("\n\nDEER OS v0.0.1\n");
        printf("===============\n\n");
        printf_set_color(COLOR_CYAN);
        printf("System Status:\n");
        printf("--------------\n");
        printf_set_color(COLOR_GREEN);
        printf("GDT: ACTIVE\n");
        printf("PAGING: ACTIVE\n");
        printf("PMM: READY\n");
        printf("IDT: ACTIVE\n");
        printf("PIC: REMAPPED\n");
        printf("PIT: INITIALIZED\n");
        printf("INTERRUPTS: ENABLED\n");
        
        printf("\n");
            
        printf_set_color(COLOR_GREEN);
        printf("All systems ready - Timer active\n");
    }
 
        
    serial_puts("[DEER] Graphics initialized successfully\n");

    serial_puts("[DEER] Entering main kernel loop...\n");
    
    while(1) {   
        asm volatile("hlt");
    }
}