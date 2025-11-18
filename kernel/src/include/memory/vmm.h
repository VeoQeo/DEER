#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "paging.h"

#define VMM_KERNEL_BASE 0xFFFFFFFF80000000
#define VMM_HHDM_OFFSET 0xFFFF800000000000

typedef struct {
    page_table_t* pml4;
    uint64_t hhdm_offset;
} vmm_space_t;

void vmm_init(volatile struct limine_hhdm_response *hhdm_response);
vmm_space_t* vmm_create_space(void);
void vmm_destroy_space(vmm_space_t *space);
void vmm_switch_space(vmm_space_t *space);

bool vmm_map_page(vmm_space_t *space, uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags);
bool vmm_unmap_page(vmm_space_t *space, uint64_t virtual_addr);
bool vmm_is_mapped(vmm_space_t *space, uint64_t virtual_addr);
uint64_t vmm_get_physical(vmm_space_t *space, uint64_t virtual_addr);

bool vmm_map_kernel(vmm_space_t *space);

void vmm_dump_mappings(vmm_space_t *space);

#endif