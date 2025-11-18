#ifndef SMP_H
#define SMP_H

#include <stdint.h>
#include <stdbool.h>
#include <limine.h>

#define CPU_STATE_UNUSED    0
#define CPU_STATE_STARTING  1
#define CPU_STATE_RUNNING   2
#define CPU_STATE_HALTED    3

#define MAX_CPUS 256

struct cpu_info {
    uint32_t id;
    uint32_t lapic_id;
    uint64_t kernel_stack;
    volatile uint32_t state;
    uint64_t gs_base;
    bool is_bsp;
} __attribute__((packed));

struct smp_state {
    struct cpu_info cpus[MAX_CPUS];
    uint32_t cpu_count;
    uint32_t bsp_id;
    bool smp_available;
    bool smp_initialized;
    volatile uint32_t started_count;
};

void smp_init(volatile struct limine_mp_response *mp_response);
void smp_start_aps(void);
uint32_t smp_get_cpu_count(void);
struct cpu_info *smp_get_current_cpu(void);
struct cpu_info *smp_get_cpu_info(uint32_t cpu_id);
bool smp_is_bsp(void);

void smp_send_init(uint32_t lapic_id);
void smp_send_startup(uint32_t lapic_id, uint8_t vector);
void smp_broadcast_ipi(uint8_t vector);
void smp_send_ipi(uint32_t lapic_id, uint8_t vector);

extern struct smp_state smp_state;

#endif // SMP_H