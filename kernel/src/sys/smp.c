#include "include/sys/smp.h"
#include "include/sys/apic.h"
#include "include/sys/gdt.h"
#include "include/interrupts/idt.h"
#include "include/memory/pmm.h"
#include "include/memory/paging.h"
#include "include/drivers/serial.h"
#include "libc/string.h"
#include "libc/stdio.h"

extern volatile struct limine_mp_request mp_request;
struct smp_state smp_state = {0};
static volatile struct limine_mp_response *mp_response = NULL;

static void ap_entry(struct limine_mp_info *info) {
    uint32_t lapic_id = info->lapic_id;
    
    serial_puts("[SMP] AP entry for LAPIC ID ");
    serial_put_hex64(lapic_id);
    serial_puts("\n");
    
    struct cpu_info *cpu = NULL;
    for (uint32_t i = 0; i < smp_state.cpu_count; i++) {
        if (smp_state.cpus[i].lapic_id == lapic_id) {
            cpu = &smp_state.cpus[i];
            break;
        }
    }
    
    if (!cpu) {
        serial_puts("[SMP] ERROR: No CPU info found for LAPIC ID ");
        serial_put_hex64(lapic_id);
        serial_puts("\n");
        return;
    }
    
    uint64_t stack_page = pmm_alloc_page();
    if (stack_page) {
        cpu->kernel_stack = stack_page + 0x1000 - 16;
        paging_map_page(cpu->kernel_stack & ~0xFFF, stack_page, 
                       PAGING_PRESENT | PAGING_WRITABLE);
    } else {
        serial_puts("[SMP] ERROR: Failed to allocate stack for AP\n");
        cpu->state = CPU_STATE_HALTED;
        return;
    }
    
    cpu->gs_base = (uint64_t)cpu;
    asm volatile("wrmsr" : : "c"(0xC0000101), 
                 "a"((uint32_t)cpu->gs_base), "d"(cpu->gs_base >> 32));
    
    gdt_load();
    idt_load();
    
    if (apic_state.apic_available) {
        lapic_write(LAPIC_SIV_REG, lapic_read(LAPIC_SIV_REG) | LAPIC_SIV_ENABLE);
        lapic_write(LAPIC_TASK_PRIO_REG, 0);
    }
    
    cpu->state = CPU_STATE_RUNNING;
    
    __sync_fetch_and_add(&smp_state.started_count, 1);
    
    serial_puts("[SMP] AP ");
    serial_put_hex64(lapic_id);
    serial_puts(" started successfully\n");
    
    while (1) {
        asm volatile("hlt");
        asm volatile("pause");
    }
}

void smp_init(volatile struct limine_mp_response *limine_mp_response) {
    serial_puts("[SMP] Initializing SMP...\n");
    mp_response = limine_mp_response;
    
    if (!mp_response) {
        serial_puts("[SMP] No MP response from bootloader\n");
        smp_state.smp_available = false;
        smp_state.cpu_count = 1;
        smp_state.cpus[0].id = 0;
        smp_state.cpus[0].lapic_id = 0; 
        smp_state.cpus[0].state = CPU_STATE_RUNNING;
        smp_state.cpus[0].is_bsp = true;
        smp_state.cpus[0].gs_base = (uint64_t)&smp_state.cpus[0];
        smp_state.started_count = 1;
        smp_state.bsp_id = 0;
        
        return;
    }
    
    smp_state.cpu_count = mp_response->cpu_count;
    smp_state.bsp_id = mp_response->bsp_lapic_id;
    smp_state.smp_available = true;
    smp_state.started_count = 1; 
    
    serial_puts("[SMP] Found ");
    char count_str[16];
    serial_puts(itoa(smp_state.cpu_count, count_str, 10));
    serial_puts(" CPUs\n");
    serial_puts("[SMP] BSP: LAPIC ID ");
    serial_put_hex64(smp_state.bsp_id);
    serial_puts("\n");
    
    for (uint32_t i = 0; i < smp_state.cpu_count && i < MAX_CPUS; i++) {
        volatile struct limine_mp_info *cpu_info = mp_response->cpus[i];
        
        smp_state.cpus[i].id = i;
        smp_state.cpus[i].lapic_id = cpu_info->lapic_id;
        smp_state.cpus[i].state = CPU_STATE_UNUSED;
        smp_state.cpus[i].is_bsp = (cpu_info->lapic_id == smp_state.bsp_id);
        
        if (smp_state.cpus[i].is_bsp) {
            smp_state.cpus[i].state = CPU_STATE_RUNNING;
            smp_state.cpus[i].gs_base = (uint64_t)&smp_state.cpus[i];
            
            serial_puts("[SMP] BSP registered: LAPIC ID ");
            serial_put_hex64(smp_state.cpus[i].lapic_id);
            serial_puts("\n");
        } else {
            serial_puts("[SMP] AP registered: LAPIC ID ");
            serial_put_hex64(smp_state.cpus[i].lapic_id);
            serial_puts("\n");
        }
    }
    
    serial_puts("[SMP] SMP initialization complete\n");
}

void smp_start_aps(void) {
    if (!smp_state.smp_available || !mp_response) {
        serial_puts("[SMP] SMP not available or no MP response\n");
        return;
    }
    
    if (smp_state.cpu_count <= 1) {
        serial_puts("[SMP] Only one CPU, no APs to start\n");
        return;
    }
    
    serial_puts("[SMP] Starting APs via Limine MP...\n");
    
    for (uint64_t i = 0; i < mp_response->cpu_count; i++) {
        volatile struct limine_mp_info *cpu = mp_response->cpus[i];
        
        if (cpu->lapic_id == smp_state.bsp_id) {
            continue;
        }
        
        if (cpu->goto_address != NULL) {
            serial_puts("[SMP] CPU ");
            serial_put_hex64(cpu->lapic_id);
            serial_puts(" already has goto_address set, skipping\n");
            continue;
        }
        
        cpu->goto_address = ap_entry;
        
        serial_puts("[SMP] Set entry point for LAPIC ID: ");
        serial_put_hex64(cpu->lapic_id);
        serial_puts("\n");
    }
    
    serial_puts("[SMP] Waiting for APs to start...\n");
    
    for (int timeout = 0; timeout < 5000000; timeout++) {
        uint32_t running_aps = 0;
        for (uint32_t i = 0; i < smp_state.cpu_count; i++) {
            if (!smp_state.cpus[i].is_bsp && 
                smp_state.cpus[i].state == CPU_STATE_RUNNING) {
                running_aps++;
            }
        }
        
        if (running_aps >= smp_state.cpu_count - 1) {
            break;
        }
        
        for (int j = 0; j < 1000; j++) {
            asm volatile("pause");
        }
        
        if (timeout % 100000 == 0) {
            serial_puts("[SMP] Still waiting for APs... ");
            char timeout_str[16];
            serial_puts(itoa(timeout / 100000, timeout_str, 10));
            serial_puts(" cycles\n");
        }
    }
    
    uint32_t running_aps = 0;
    for (uint32_t i = 0; i < smp_state.cpu_count; i++) {
        if (!smp_state.cpus[i].is_bsp && 
            smp_state.cpus[i].state == CPU_STATE_RUNNING) {
            running_aps++;
        }
    }
    
    serial_puts("[SMP] AP startup completed: ");
    char running_str[16], total_str[16];
    serial_puts(itoa(running_aps, running_str, 10));
    serial_puts("/");
    serial_puts(itoa(smp_state.cpu_count - 1, total_str, 10));
    serial_puts(" APs running\n");
    
    smp_state.started_count = 1 + running_aps; 
}

void smp_send_init(uint32_t lapic_id) {
    if (!apic_state.apic_available) return;
    
    lapic_write(LAPIC_ICR1_REG, lapic_id << 24);
    lapic_write(LAPIC_ICR2_REG, 0x00004500); 
    while (lapic_read(LAPIC_ICR2_REG) & (1 << 12));
}

void smp_send_startup(uint32_t lapic_id, uint8_t vector) {
    if (!apic_state.apic_available) return;
    
    lapic_write(LAPIC_ICR1_REG, lapic_id << 24);
    lapic_write(LAPIC_ICR2_REG, 0x00004600 | vector); 
    while (lapic_read(LAPIC_ICR2_REG) & (1 << 12));
}

void smp_send_ipi(uint32_t lapic_id, uint8_t vector) {
    if (!apic_state.apic_available) return;
    
    lapic_write(LAPIC_ICR1_REG, lapic_id << 24);
    lapic_write(LAPIC_ICR2_REG, 0x00004000 | vector); 
    while (lapic_read(LAPIC_ICR2_REG) & (1 << 12));
}

void smp_broadcast_ipi(uint8_t vector) {
    if (!apic_state.apic_available) return;
    
    lapic_write(LAPIC_ICR1_REG, 0xFF << 24); 
    lapic_write(LAPIC_ICR2_REG, 0x00004000 | vector); 
    while (lapic_read(LAPIC_ICR2_REG) & (1 << 12));
}

uint32_t smp_get_cpu_count(void) {
    return smp_state.cpu_count;
}

struct cpu_info *smp_get_current_cpu(void) {
    if (smp_is_bsp()) {
        return &smp_state.cpus[0];
    }
    
    uint64_t gs_base;
    asm volatile("rdmsr" : "=a"(gs_base) : "c"(0xC0000101) : "edx");
    return (struct cpu_info*)gs_base;
}

struct cpu_info *smp_get_cpu_info(uint32_t cpu_id) {
    if (cpu_id >= smp_state.cpu_count) return NULL;
    return &smp_state.cpus[cpu_id];
}

bool smp_is_bsp(void) {
    if (smp_state.cpu_count == 0) return true;
    
    if (apic_state.apic_available) {
        uint32_t current_lapic_id = lapic_read(LAPIC_ID_REG) >> 24;
        return (current_lapic_id == smp_state.bsp_id);
    }
    
    return true;
}