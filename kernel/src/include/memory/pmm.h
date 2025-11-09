#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

void pmm_init(volatile struct limine_memmap_response *memmap_response,
              volatile struct limine_hhdm_response *hhdm_response);
uint64_t pmm_alloc_page(void);
void pmm_free_page(uint64_t page);
uint64_t pmm_get_total_memory(void);
uint64_t pmm_get_free_memory(void);
uint64_t pmm_get_used_memory(void);
void pmm_dump_memory_map(void);

#endif // PMM_H