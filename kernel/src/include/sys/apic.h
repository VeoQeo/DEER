#ifndef APIC_H
#define APIC_H

#include <stdint.h>
#include <stdbool.h>
#include <limine.h>
#include "../interrupts/isr.h" 

#define LAPIC_ID_REG 0x20
#define LAPIC_VERSION_REG 0x30
#define LAPIC_TASK_PRIO_REG 0x80
#define LAPIC_ARBITR_PRIO_REG 0x90
#define LAPIC_PROC_PRIO_REG 0xA0
#define LAPIC_EOI_REG 0xB0
#define LAPIC_LOGICAL_DEST_REG 0xD0
#define LAPIC_DEST_FORMAT_REG 0xE0
#define LAPIC_SIV_REG 0xF0
#define LAPIC_ISR_BASE 0x100
#define LAPIC_TMR_BASE 0x180
#define LAPIC_IRR_BASE 0x200
#define LAPIC_ERROR_STATUS_REG 0x280
#define LAPIC_LVT_CMCI_REG 0x2F0
#define LAPIC_ICR1_REG 0x300
#define LAPIC_ICR2_REG 0x310
#define LAPIC_LVT_TIMER 0x320
#define LAPIC_LVT_THERMAL 0x330
#define LAPIC_LVT_PERF 0x340
#define LAPIC_LVT_LINT0 0x350
#define LAPIC_LVT_LINT1 0x360
#define LAPIC_LVT_ERROR 0x370
#define LAPIC_TIMER_INITIAL 0x380
#define LAPIC_TIMER_CURRENT 0x390
#define LAPIC_TIMER_DIVIDER 0x3E0
#define LAPIC_EOI_ACK 0x0
#define LAPIC_SIV_ENABLE 0x100
#define LAPIC_TIMER_PERIODIC 0x20000

#define IOAPIC_REDTBL_BASE 0x10

#define HPET_CAPABILITIES 0x00
#define HPET_CONFIG 0x10
#define HPET_MAIN_COUNTER 0xF0
#define HPET_TIMER_CONFIG(n) (0x100 + 0x20 * (n))
#define HPET_TIMER_COMPARATOR(n) (0x108 + 0x20 * (n))

struct ioapic_regs {
    volatile uint32_t ioregsel;
    volatile uint32_t reserved[3];
    volatile uint32_t iowin;
} __attribute__((packed));

struct hpet_regs {
    volatile uint64_t capabilities;
    volatile uint64_t reserved;
    volatile uint64_t config;
    volatile uint64_t reserved2;
    volatile uint64_t main_counter;
    volatile uint64_t reserved3;
    volatile uint64_t config2;
    volatile uint64_t reserved4[5];
    volatile uint64_t timer_config[32];
    volatile uint64_t timer_comparator[32];
    volatile uint64_t reserved5[416];
} __attribute__((packed));

struct apic_state {
    void* lapic_base;
    struct ioapic_regs* ioapic_base;
    struct hpet_regs* hpet_base;
    uint64_t hpet_frequency;
    bool apic_available;
    bool ioapic_available;
    bool hpet_available;
};

extern struct apic_state apic_state;
extern volatile uint64_t lapic_ticks; 

void apic_init(volatile struct limine_hhdm_response *hhdm_response);
void lapic_init(volatile struct limine_hhdm_response *hhdm_response);
void ioapic_init(volatile struct limine_hhdm_response *hhdm_response);
void hpet_init(volatile struct limine_hhdm_response *hhdm_response);

uint32_t lapic_read(uint32_t reg);
void lapic_write(uint32_t reg, uint32_t value);
void lapic_eoi(void);

uint32_t ioapic_read(uint32_t reg);
void ioapic_write(uint32_t reg, uint32_t value);
void ioapic_redirect_irq(uint8_t irq, uint8_t vector, uint32_t delivery_mode);

uint64_t hpet_read(uint64_t reg);
void hpet_write(uint64_t reg, uint64_t value);

uint64_t hpet_get_frequency(void);
void hpet_sleep_ms(uint64_t milliseconds);
void hpet_sleep_us(uint64_t microseconds);

bool apic_available(void);
void apic_disable_pic(void);

void lapic_timer_init(uint8_t vector, uint32_t frequency);
void lapic_timer_stop(void);

uint64_t lapic_get_ticks(void);
void lapic_sleep_ms(uint64_t ms);
void lapic_sleep_us(uint64_t us);
void lapic_timer_handler(struct registers *regs); 
void lapic_timer_handler_for_sleep(struct registers *regs); 

#endif // APIC_H