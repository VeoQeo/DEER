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
#include "include/simd/simd.h"
#include "include/memory/pmm.h"
#include "include/memory/paging.h"
#include "include/memory/vmm.h"
#include "include/memory/heap.h"
#include "include/sys/acpi.h"
#include "include/sys/apic.h"
#include "include/sys/smp.h"
#include "include/tasking/task.h"

// crypto
#include "include/crypto/aes.h"

extern struct apic_state apic_state;

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
volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_mp_request mp_request = {
    .id = LIMINE_MP_REQUEST,
    .revision = 0,
    .response = NULL,
    .flags = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

static volatile struct limine_hhdm_response *hhdm_response = NULL;
static volatile struct limine_memmap_response *memmap_response = NULL;
volatile struct limine_rsdp_response *rsdp_response = NULL;

void initialize_memory_subsystems(void) {
    serial_puts("[DEER] Initializing Physical Memory Manager...\n");
    pmm_init(memmap_response, hhdm_response);
    serial_puts("[DEER] PMM initialized\n");

    serial_puts("[DEER] Initializing Paging...\n");
    paging_init(hhdm_response);
    serial_puts("[DEER] Paging initialized\n");

    serial_puts("[DEER] Initializing Kernel Heap...\n");
    heap_init();
    serial_puts("[DEER] Heap initialized\n");

    serial_puts("[DEER] Initializing Virtual Memory Manager...\n");
    vmm_init(hhdm_response);
    serial_puts("[DEER] VMM initialized\n");
}

void initialize_subsystems(void) {
    serial_puts("[DEER] Initializing GDT...\n");
    gdt_init();
    gdt_load();
    serial_puts("[DEER] GDT initialized\n");

    initialize_memory_subsystems();
    
    serial_puts("[DEER] Initializing PIC...\n");
    pic_remap();
    serial_puts("[DEER] PIC remapped\n");

    serial_puts("[DEER] Initializing IDT...\n");
    idt_init();
    serial_puts("[DEER] IDT initialized\n");

    serial_puts("[DEER] Initializing ACPI...\n");
    acpi_init(hhdm_response); 
    serial_puts("[DEER] ACPI initialized\n");

    serial_puts("[DEER] Initializing APIC...\n");
    apic_init(hhdm_response);
    serial_puts("[DEER] APIC initialized\n");

    serial_puts("[DEER] Initializing SMP...\n");
    smp_init(mp_request.response);
    serial_puts("[DEER] SMP initialized\n");

    serial_puts("[DEER] Initializing IRQ...\n");
    irq_init();
    serial_puts("[DEER] IRQ initialized\n");

    if (apic_state.apic_available && apic_state.ioapic_available) {
        serial_puts("[DEER] Initializing LAPIC timer...\n");
        lapic_timer_init(32, 1000); // IRQ0, 1000Hz
        serial_puts("[DEER] LAPIC timer initialized\n");
    } else {
        serial_puts("[DEER] Initializing PIT...\n");
        pit_init();
        serial_puts("[DEER] PIT initialized\n");
    }

    serial_puts("[DEER] Installing timer handler...\n");
    if (apic_state.apic_available && apic_state.ioapic_available) {
        irq_install_handler(0, lapic_timer_handler); 
        serial_puts("[DEER] LAPIC timer handler installed\n");
    } else {
        irq_install_handler(0, pit_timer_handler); 
        serial_puts("[DEER] PIT timer handler installed\n");
    }
}

void display_system_info(struct limine_framebuffer *fb) {
    if (!fb) return;
    
    printf_init_with_framebuffer(fb);
    printf_set_color(COLOR_GREEN);
    printf_set_bg_color(COLOR_BLACK);
        
    printf_clear();
    printf_set_color(COLOR_ORANGE);
    printf("\n\nDEER OS v0.0.1 - MULTITASKING TEST\n");
    printf("==================================\n\n");
    printf_set_color(COLOR_CYAN);
    printf("System Status:\n");
    printf("--------------\n");
    printf_set_color(COLOR_GREEN);
    printf("GDT: ACTIVE\n");
    printf("IDT: ACTIVE\n");
    printf("PIC: REMAPPED\n");
    printf("INTERRUPTS: ENABLED\n");
    printf("PMM: ACTIVE (%d MB free)\n", pmm_get_free_memory() / 1024 / 1024);
    printf("PAGING: ACTIVE\n");
    printf("HEAP: ACTIVE (%d KB used)\n", heap_get_used_size() / 1024);
    printf("VMM: ACTIVE\n");
    printf("ACPI: ACTIVE\n");
    printf("APIC: ACTIVE\n");

    print_cpu_features();
    printf("\n");
    
    printf_set_color(COLOR_GREEN);

    
    printf_set_color(COLOR_CYAN);
    printf("SMP Status:\n");
    printf("-----------\n");
    printf_set_color(COLOR_GREEN);
    printf("SMP: %s\n", smp_state.smp_available ? "AVAILABLE" : "UNAVAILABLE");
    printf("CPUs: %d/%d running\n", smp_state.started_count, smp_state.cpu_count);
    printf("BSP: LAPIC ID %d\n", smp_state.bsp_id);
    
    for (uint32_t i = 0; i < smp_state.cpu_count && i < 8; i++) {
        printf("CPU%d: LAPIC ID %d, State: ", 
               smp_state.cpus[i].id, 
               smp_state.cpus[i].lapic_id);
        
        switch(smp_state.cpus[i].state) {
            case CPU_STATE_UNUSED: printf("UNUSED"); break;
            case CPU_STATE_STARTING: printf("STARTING"); break;
            case CPU_STATE_RUNNING: printf("RUNNING"); break;
            case CPU_STATE_HALTED: printf("HALTED"); break;
            default: printf("UNKNOWN"); break;
        }
        
        if (smp_state.cpus[i].is_bsp) {
            printf(" [BSP]");
        }
        printf("\n");
    }

    lapic_sleep_ms(3000); 

    printf("\n3 seconds have passed (using LAPIC sleep).");
    
    printf("\nSystem operational. Press Ctrl+Alt+G to release mouse from QEMU.\n");
}

void kernel_main(void) {

    enable_sse();
    serial_init();
    serial_puts("[DEER] Kernel started\n");
    serial_puts("[DEER] Detecting CPU features...\n");

    // we load aes256 gcm module load for test build and start
    // this is just test :D
    // no ecrypt something, lol
    {
        uint8_t test_key[16] = {
            0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
            0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
        };
        struct AES_ctx aes_ctx;
        AES_init_ctx(&aes_ctx, test_key);
        serial_puts("[DEER] AES context initialized\n");
    }

    serial_puts("[DEER] Initializing SSE...\n");
    if (enable_sse_safe() != 0) {
        serial_puts("[DEER] ERROR: Failed to initialize SSE!\n");
    }

    hhdm_response = hhdm_request.response;
    memmap_response = memmap_request.response;
    rsdp_response = rsdp_request.response;
    
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

    if (!rsdp_response) {
        serial_puts("[DEER] WARNING: No RSDP response from bootloader!\n");
    } else {
        serial_puts("[DEER] RSDP available\n");
    }

    initialize_subsystems();

    if (smp_state.smp_available && smp_state.cpu_count > 1) {
        serial_puts("[DEER] Starting application processors...\n");
        smp_start_aps();
    }

    serial_puts("[DEER] Enabling interrupts...\n");
    idt_load();
    serial_puts("[DEER] Interrupts enabled\n");

    struct limine_framebuffer *fb = NULL;
    if (framebuffer_request.response && framebuffer_request.response->framebuffer_count > 0) {
        fb = framebuffer_request.response->framebuffers[0];
        display_system_info(fb);
        serial_puts("[DEER] Graphics initialized successfully\n");
    }

    serial_puts("[DEER] Entering kernel main loop...\n");

    while (1) {  
        asm volatile("hlt");
    }
}