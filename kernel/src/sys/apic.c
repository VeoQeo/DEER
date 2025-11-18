#include "include/sys/apic.h"
#include "include/sys/acpi.h"
#include "include/drivers/io.h"
#include "include/drivers/serial.h"
#include "include/drivers/pic.h"
#include "include/interrupts/isr.h"
#include "include/interrupts/irq.h" 
#include "libc/stdio.h"
#include "libc/string.h"
#include "include/memory/paging.h"
#include "include/tasking/task.h"

// Макрос для чтения из MMIO APIC
#define APIC_READ(reg) (*(volatile uint32_t*)(apic_state.lapic_base + (reg)))
// Макрос для записи в MMIO APIC
#define APIC_WRITE(reg, val) (*(volatile uint32_t*)(apic_state.lapic_base + (reg)) = (val))

// Определения регистров ICR
#define APIC_ICR1 0x310
#define APIC_ICR2 0x300
#define APIC_ICR_DELIVERY_MODE_SHIFT 8
#define APIC_ICR_DEST_MODE_SHIFT 11
#define APIC_ICR_LEVEL_SHIFT 14
#define APIC_ICR_TRIGGER_MODE_SHIFT 15
#define APIC_ICR_DEST_SHORTHAND_SHIFT 18
#define APIC_ICR_DELIVERY_STATUS_BIT 12

#define APIC_DELIVERY_MODE_INIT  5
#define APIC_DELIVERY_MODE_SIPI  6
#define APIC_LEVEL_DEASSERT      0
#define APIC_LEVEL_ASSERT        1
#define APIC_TRIGGER_MODE_EDGE   0
#define APIC_TRIGGER_MODE_LEVEL  1
#define APIC_DEST_SHORTHAND_NONE 0
#define APIC_DEST_SHORTHAND_SELF 1
#define APIC_DEST_SHORTHAND_ALL  2
#define APIC_DEST_SHORTHAND_ALL_EXCEPT_SELF 3

struct apic_state apic_state = {0};

volatile uint64_t lapic_ticks = 0; 

uint64_t lapic_get_ticks(void) {
    return lapic_ticks;
}

void lapic_timer_handler(struct registers *regs) {
    (void)regs; 
    lapic_ticks++;
}

static volatile bool lapic_sleeping = false;
static volatile uint64_t sleep_end_ticks = 0;

void lapic_sleep_ms(uint64_t ms) {
    if (!apic_state.apic_available) return;

    uint64_t current_ticks = lapic_get_ticks();
    uint64_t target_ticks = current_ticks + ms;

    if (target_ticks <= current_ticks) return; 

    lapic_sleeping = true;
    sleep_end_ticks = target_ticks;

    while (lapic_sleeping && lapic_get_ticks() < target_ticks) {
        asm volatile("hlt");
    }

    lapic_sleeping = false; 
}

void lapic_sleep_us(uint64_t us) {
    uint64_t ms = (us + 999) / 1000; 
    lapic_sleep_ms(ms);
}

void lapic_timer_handler_for_sleep(struct registers *regs) {
    (void)regs;
    lapic_ticks++;

    if (lapic_sleeping && lapic_ticks >= sleep_end_ticks) {
        lapic_sleeping = false;
    }
}

bool apic_available(void) {
    uint32_t eax, edx;
    asm volatile("cpuid" : "=a"(eax), "=d"(edx) : "a"(1) : "ecx", "ebx");
    return (edx & (1 << 9));
}

uint32_t lapic_read(uint32_t reg) {
    if (!apic_state.lapic_base || !apic_state.apic_available) {
        serial_puts("[APIC] ERROR: Attempt to read LAPIC when not initialized\n");
        return 0;
    }
    volatile uint32_t* lapic_ptr = (volatile uint32_t*)apic_state.lapic_base;
    return lapic_ptr[reg / sizeof(uint32_t)];
}

void lapic_write(uint32_t reg, uint32_t value) {
    if (!apic_state.lapic_base || !apic_state.apic_available) {
        serial_puts("[APIC] ERROR: Attempt to write LAPIC when not initialized\n");
        return;
    }
    volatile uint32_t* lapic_ptr = (volatile uint32_t*)apic_state.lapic_base;
    lapic_ptr[reg / sizeof(uint32_t)] = value;
}

void lapic_eoi(void) {
    lapic_write(LAPIC_EOI_REG, LAPIC_EOI_ACK);
}

void lapic_init(volatile struct limine_hhdm_response *hhdm_response) {
    if (!apic_state.apic_available) {
        serial_puts("[APIC] APIC not available, falling back to PIC\n");
        return;
    }

    if (!hhdm_response) {
        serial_puts("[APIC] ERROR: No HHDM response provided!\n");
        apic_state.apic_available = false;
        return;
    }

    serial_puts("[APIC] APIC supported, initializing components...\n");
    serial_puts("[APIC] HHDM offset: 0x");
    char buffer[32];
    serial_puts(itoa(hhdm_response->offset, buffer, 16));
    serial_puts("\n");

    uint32_t eax, edx;
    asm volatile("rdmsr" : "=a"(eax), "=d"(edx) : "c"(0x1B));
    uint32_t lapic_physical_base = (eax & 0xFFFFF000);
    serial_puts("[APIC] LAPIC physical base from MSR: 0x");
    serial_puts(itoa(lapic_physical_base, buffer, 16));
    serial_puts("\n");

    uint64_t lapic_page_aligned_phys = (uint64_t)lapic_physical_base & ~0xFFF;
    uint64_t lapic_page_aligned_virt = lapic_page_aligned_phys + hhdm_response->offset;

    serial_puts("[APIC] Attempting to map LAPIC page:\n");
    serial_puts("  Physical: 0x");
    serial_puts(itoa(lapic_page_aligned_phys, buffer, 16));
    serial_puts("\n  Virtual:  0x");
    serial_puts(itoa(lapic_page_aligned_virt, buffer, 16));
    serial_puts("\n");

    #ifdef PAGING_NO_EXECUTE
        #define LAPIC_PAGING_FLAGS (PAGING_PRESENT | PAGING_WRITABLE | PAGING_NO_EXECUTE)
    #else
        #define LAPIC_PAGING_FLAGS (PAGING_PRESENT | PAGING_WRITABLE)
    #endif

    if (!paging_map_page(lapic_page_aligned_virt, lapic_page_aligned_phys, LAPIC_PAGING_FLAGS)) {
        serial_puts("[APIC] ERROR: Failed to map LAPIC physical page to virtual memory!\n");
        apic_state.apic_available = false;
        return;
    }
    serial_puts("[APIC] LAPIC page mapped successfully.\n");

    apic_state.lapic_base = (void*)(lapic_page_aligned_virt + ((uint64_t)lapic_physical_base & 0xFFF));

    serial_puts("[APIC] LAPIC virtual base: 0x");
    serial_puts(itoa((uint64_t)apic_state.lapic_base, buffer, 16));
    serial_puts("\n");

    uint32_t spurious_vector = 0xFF;
    lapic_write(LAPIC_SIV_REG, lapic_read(LAPIC_SIV_REG) | LAPIC_SIV_ENABLE | spurious_vector);

    serial_puts("[APIC] LAPIC initialized and enabled\n");
}

uint32_t ioapic_read(uint32_t reg) {
    if (!apic_state.ioapic_base) return 0;
    apic_state.ioapic_base->ioregsel = reg;
    return apic_state.ioapic_base->iowin;
}

void ioapic_write(uint32_t reg, uint32_t value) {
    if (!apic_state.ioapic_base) return;
    apic_state.ioapic_base->ioregsel = reg;
    apic_state.ioapic_base->iowin = value;
}

void ioapic_redirect_irq(uint8_t irq, uint8_t vector, uint32_t delivery_mode) {
    if (!apic_state.ioapic_available) return;

    uint32_t low_index = IOAPIC_REDTBL_BASE + (irq * 2);
    uint32_t high_index = low_index + 1;

    uint32_t value = vector | delivery_mode;

    ioapic_write(low_index, value);
    ioapic_write(high_index, 0);
}

void ioapic_init(volatile struct limine_hhdm_response *hhdm_response) {
    if (!acpi_state.madt) {
        serial_puts("[APIC] No MADT found, cannot initialize IOAPIC\n");
        return;
    }

    uint8_t* madt_start = (uint8_t*)acpi_state.madt;
    uint8_t* madt_end = madt_start + acpi_state.madt->header.length;
    uint8_t* ptr = madt_start + sizeof(struct acpi_madt);

    while (ptr < madt_end) {
        struct madt_entry_header* header = (struct madt_entry_header*)ptr;

        if (header->type == MADT_ENTRY_IOAPIC) {
            struct madt_ioapic* ioapic = (struct madt_ioapic*)header;

            if (hhdm_response) {
                apic_state.ioapic_base = (struct ioapic_regs*)((uint64_t)ioapic->ioapic_addr + hhdm_response->offset);
                apic_state.ioapic_available = true;

                serial_puts("[APIC] Found IOAPIC at physical: 0x");
                char buffer[32];
                serial_puts(itoa(ioapic->ioapic_addr, buffer, 16));
                serial_puts(", virtual: 0x");
                serial_puts(itoa((uint64_t)apic_state.ioapic_base, buffer, 16));
                serial_puts("\n");
            } else {
                serial_puts("[APIC] ERROR: No HHDM response for IOAPIC mapping!\n");
            }
            break;
        }

        ptr += header->length;
    }

    if (!apic_state.ioapic_available) {
        serial_puts("[APIC] No IOAPIC found in MADT\n");
        return;
    }

    serial_puts("[APIC] IOAPIC initialized\n");
}

uint64_t hpet_read(uint64_t reg) {
    if (!apic_state.hpet_base) return 0;
    return *((volatile uint64_t*)((uint8_t*)apic_state.hpet_base + reg));
}

void hpet_write(uint64_t reg, uint64_t value) {
    if (!apic_state.hpet_base) return;
    *((volatile uint64_t*)((uint8_t*)apic_state.hpet_base + reg)) = value;
}

uint64_t hpet_get_frequency(void) {
    return apic_state.hpet_frequency;
}

void hpet_init(volatile struct limine_hhdm_response *hhdm_response) {
    if (!acpi_state.hpet) {
        serial_puts("[HPET] No HPET table found\n");
        return;
    }

    if (hhdm_response) {
        apic_state.hpet_base = (struct hpet_regs*)((uint64_t)acpi_state.hpet->base_address.address + hhdm_response->offset);

        serial_puts("[HPET] HPET at physical: 0x");
        char buffer[32];
        serial_puts(itoa(acpi_state.hpet->base_address.address, buffer, 16));
        serial_puts(", virtual: 0x");
        serial_puts(itoa((uint64_t)apic_state.hpet_base, buffer, 16));
        serial_puts("\n");
    } else {
        serial_puts("[HPET] ERROR: No HHDM response for HPET mapping!\n");
        return;
    }

    uint64_t capabilities = hpet_read(HPET_CAPABILITIES);
    apic_state.hpet_frequency = (capabilities >> 32) & 0xFFFFFFFF;

    serial_puts("[HPET] HPET frequency: ");
    char buffer[32];
    serial_puts(itoa(apic_state.hpet_frequency, buffer, 10));
    serial_puts(" femtoseconds per tick\n");

    hpet_write(HPET_CONFIG, hpet_read(HPET_CONFIG) | 1);
    hpet_write(HPET_MAIN_COUNTER, 0);

    apic_state.hpet_available = true;
    serial_puts("[HPET] HPET initialized and enabled\n");
}

void hpet_sleep_ms(uint64_t milliseconds) {
    if (!apic_state.hpet_available) return;

    uint64_t target = hpet_read(HPET_MAIN_COUNTER) +
                     (milliseconds * 1000000000000ULL) / apic_state.hpet_frequency;

    while (hpet_read(HPET_MAIN_COUNTER) < target) {
        asm volatile("pause");
    }
}

void hpet_sleep_us(uint64_t microseconds) {
    if (!apic_state.hpet_available) return;

    uint64_t target = hpet_read(HPET_MAIN_COUNTER) +
                     (microseconds * 1000000000ULL) / apic_state.hpet_frequency;

    while (hpet_read(HPET_MAIN_COUNTER) < target) {
        asm volatile("pause");
    }
}

void apic_disable_pic(void) {
    outb(0xA1, 0xFF);
    outb(0x21, 0xFF);

    serial_puts("[APIC] PIC disabled\n");
}
// Вспомогательная функция для отправки IPI
static void lapic_send_ipi(uint32_t icr1_val, uint32_t icr2_val) {
    // Ждем, пока предыдущий IPI не будет отправлен
    while (APIC_READ(APIC_ICR1) & (1 << APIC_ICR_DELIVERY_STATUS_BIT)) {
         // asm volatile("pause"); // Опционально для эффективного ожидания
    }
    APIC_WRITE(APIC_ICR2, icr2_val); // Сначала ICR2 (Destination)
    APIC_WRITE(APIC_ICR1, icr1_val); // Потом ICR1 (Vector, Delivery Mode, Level, Trigger Mode)
}


void lapic_send_init(uint32_t lapic_id) {
    uint32_t icr1 = (APIC_DELIVERY_MODE_INIT << APIC_ICR_DELIVERY_MODE_SHIFT) |
                    (APIC_LEVEL_ASSERT << APIC_ICR_LEVEL_SHIFT) |
                    (APIC_TRIGGER_MODE_EDGE << APIC_ICR_TRIGGER_MODE_SHIFT) |
                    (APIC_DEST_SHORTHAND_NONE << APIC_ICR_DEST_SHORTHAND_SHIFT);
    uint32_t icr2 = (lapic_id & 0xFF) << 24; // Destination Field

    serial_puts("[APIC] Sending INIT IPI to LAPIC ID ");
    char buf[16];
    serial_puts(itoa(lapic_id, buf, 10));
    serial_puts("\n");
    lapic_send_ipi(icr1, icr2);
}

void lapic_send_startup(uint32_t lapic_id, uint8_t vector) { // vector = target_page >> 12
    uint32_t icr1 = (APIC_DELIVERY_MODE_SIPI << APIC_ICR_DELIVERY_MODE_SHIFT) |
                    (APIC_LEVEL_ASSERT << APIC_ICR_LEVEL_SHIFT) |
                    (APIC_TRIGGER_MODE_EDGE << APIC_ICR_TRIGGER_MODE_SHIFT) |
                    (APIC_DEST_SHORTHAND_NONE << APIC_ICR_DEST_SHORTHAND_SHIFT) |
                    (vector & 0xFF);
    uint32_t icr2 = (lapic_id & 0xFF) << 24; // Destination Field

    serial_puts("[APIC] Sending SIPI IPI to LAPIC ID ");
    char buf[16];
    serial_puts(itoa(lapic_id, buf, 10));
    serial_puts(" with vector 0x");
    serial_put_hex8(vector); // или itoa(vector, buf, 16);
    serial_puts("\n");
    lapic_send_ipi(icr1, icr2);
}

void lapic_timer_init(uint8_t vector, uint32_t frequency) {
    if (!apic_state.apic_available) return;

    serial_puts("[APIC] Configuring LAPIC timer for ");
    char buffer[32];
    serial_puts(itoa(frequency, buffer, 10));
    serial_puts(" Hz...\n");

    lapic_write(LAPIC_TIMER_DIVIDER, 0x3); 

    lapic_write(LAPIC_LVT_TIMER, vector | LAPIC_TIMER_PERIODIC);

    uint32_t assumed_lapic_freq = 1000000000; // 1 GHz 
    uint32_t divisor = 16; // Из-за lapic_write(LAPIC_TIMER_DIVIDER, 0x3);

    uint64_t temp_calc = (uint64_t)assumed_lapic_freq / divisor;
    if (frequency == 0 || temp_calc / frequency == 0) {
        serial_puts("[APIC] ERROR: Invalid frequency for LAPIC timer!\n");
        return;
    }

    uint32_t initial_count = (uint32_t)(temp_calc / frequency) - 1;

    if (initial_count == 0) {
        serial_puts("[APIC] ERROR: Calculated initial count is zero, frequency too high!\n");
        return;
    }

    lapic_write(LAPIC_TIMER_INITIAL, initial_count);

    serial_puts("[APIC] LAPIC timer configured with initial count: ");
    serial_puts(itoa(initial_count, buffer, 10));
    serial_puts(" (for ~");
    serial_puts(itoa(frequency, buffer, 10));
    serial_puts(" Hz)\n");
}

void lapic_timer_stop(void) {
    if (!apic_state.apic_available) return;
    lapic_write(LAPIC_LVT_TIMER, 0x10000); 
}

void apic_init(volatile struct limine_hhdm_response *hhdm_response) {
    memset(&apic_state, 0, sizeof(apic_state));

    serial_puts("[APIC] Initializing APIC subsystem...\n");

    apic_state.apic_available = apic_available();

    if (!apic_state.apic_available) {
        serial_puts("[APIC] APIC not supported, using PIC mode\n");
        return;
    }

    if (!hhdm_response) {
        serial_puts("[APIC] ERROR: No HHDM response provided!\n");
        apic_state.apic_available = false;
        return;
    }

    lapic_init(hhdm_response);

    if (acpi_state.acpi_available) {
        ioapic_init(hhdm_response);
        hpet_init(hhdm_response);

        if (apic_state.ioapic_available) {
            apic_disable_pic();
            serial_puts("[APIC] APIC mode enabled, PIC disabled\n");
        } else {
            serial_puts("[APIC] Using hybrid mode (PIC + LAPIC)\n");
        }
    } else {
        serial_puts("[APIC] ACPI not available, using PIC mode\n");
    }

    serial_puts("[APIC] APIC initialization complete\n");
}