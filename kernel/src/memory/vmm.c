#include "include/memory/vmm.h"
#include "include/memory/heap.h"
#include "include/memory/pmm.h"
#include "include/drivers/serial.h"
#include "libc/string.h"
#include "libc/stdio.h"
#include <limine.h>

static vmm_space_t kernel_space = {0};
static uint64_t hhdm_offset = 0;

void vmm_init(volatile struct limine_hhdm_response *hhdm_response) {
    serial_puts("[VMM] Initializing...\n");
    
    if (!hhdm_response) {
        serial_puts("[VMM] ERROR: No HHDM response!\n");
        return;
    }
    
    hhdm_offset = hhdm_response->offset;
    
    uint64_t cr3 = paging_get_cr3();
    kernel_space.pml4 = (page_table_t*)(cr3 + hhdm_offset);
    kernel_space.hhdm_offset = hhdm_offset;
    
    serial_puts("[VMM] Ready\n");
}

vmm_space_t* vmm_create_space(void) {
    uint64_t pml4_phys = pmm_alloc_page();
    if (!pml4_phys) return NULL;
    
    vmm_space_t* space = (vmm_space_t*)kmalloc(sizeof(vmm_space_t));
    if (!space) {
        pmm_free_page(pml4_phys);
        return NULL;
    }
    
    space->pml4 = (page_table_t*)(pml4_phys + hhdm_offset);
    space->hhdm_offset = hhdm_offset;
    
    memset(space->pml4, 0, PAGE_SIZE_4K);
    
    vmm_map_kernel(space);
    
    return space;
}

void vmm_destroy_space(vmm_space_t *space) {
    if (!space) return;
    
    for (uint64_t i = 0; i < 256; i++) {
        if (space->pml4->entries[i] & PAGING_PRESENT) {
            page_table_t* pdp = (page_table_t*)((space->pml4->entries[i] & ~0xFFF) + hhdm_offset);
            for (uint64_t j = 0; j < 512; j++) {
                if (pdp->entries[j] & PAGING_PRESENT) {
                    page_table_t* pd = (page_table_t*)((pdp->entries[j] & ~0xFFF) + hhdm_offset);
                    for (uint64_t k = 0; k < 512; k++) {
                        if (pd->entries[k] & PAGING_PRESENT) {
                            page_table_t* pt = (page_table_t*)((pd->entries[k] & ~0xFFF) + hhdm_offset);
                            for (uint64_t l = 0; l < 512; l++) {
                                if (pt->entries[l] & PAGING_PRESENT) {
                                    pmm_free_page(pt->entries[l] & ~0xFFF);
                                }
                            }
                            pmm_free_page(pd->entries[k] & ~0xFFF);
                        }
                    }
                    pmm_free_page(pdp->entries[j] & ~0xFFF);
                }
            }
            pmm_free_page(space->pml4->entries[i] & ~0xFFF);
        }
    }
    
    pmm_free_page((uint64_t)space->pml4 - hhdm_offset);
    kfree(space);
}

void vmm_switch_space(vmm_space_t *space) {
    if (!space) return;
    uint64_t phys = (uint64_t)space->pml4 - hhdm_offset;
    paging_load_cr3(phys);
}

bool vmm_map_page(vmm_space_t *space, uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags) {
    if (!space) return false;
    
    uint64_t pml4_index = (virtual_addr >> 39) & 0x1FF;
    uint64_t pdp_index = (virtual_addr >> 30) & 0x1FF;
    uint64_t pd_index = (virtual_addr >> 21) & 0x1FF;
    uint64_t pt_index = (virtual_addr >> 12) & 0x1FF;
    
    page_table_t* pdp;
    if (!(space->pml4->entries[pml4_index] & PAGING_PRESENT)) {
        uint64_t pdp_phys = pmm_alloc_page();
        if (!pdp_phys) return false;
        pdp = (page_table_t*)(pdp_phys + hhdm_offset);
        memset(pdp, 0, PAGE_SIZE_4K);
        space->pml4->entries[pml4_index] = pdp_phys | PAGING_PRESENT | PAGING_WRITABLE | (flags & PAGING_USER ? PAGING_USER : 0);
    } else {
        pdp = (page_table_t*)((space->pml4->entries[pml4_index] & ~0xFFF) + hhdm_offset);
    }
    
    page_table_t* pd;
    if (!(pdp->entries[pdp_index] & PAGING_PRESENT)) {
        uint64_t pd_phys = pmm_alloc_page();
        if (!pd_phys) return false;
        pd = (page_table_t*)(pd_phys + hhdm_offset);
        memset(pd, 0, PAGE_SIZE_4K);
        pdp->entries[pdp_index] = pd_phys | PAGING_PRESENT | PAGING_WRITABLE | (flags & PAGING_USER ? PAGING_USER : 0);
    } else {
        pd = (page_table_t*)((pdp->entries[pdp_index] & ~0xFFF) + hhdm_offset);
    }
    
    page_table_t* pt;
    if (!(pd->entries[pd_index] & PAGING_PRESENT)) {
        uint64_t pt_phys = pmm_alloc_page();
        if (!pt_phys) return false;
        pt = (page_table_t*)(pt_phys + hhdm_offset);
        memset(pt, 0, PAGE_SIZE_4K);
        pd->entries[pd_index] = pt_phys | PAGING_PRESENT | PAGING_WRITABLE | (flags & PAGING_USER ? PAGING_USER : 0);
    } else {
        pt = (page_table_t*)((pd->entries[pd_index] & ~0xFFF) + hhdm_offset);
    }
    
    pt->entries[pt_index] = physical_addr | flags | PAGING_PRESENT;
    return true;
}

bool vmm_map_kernel(vmm_space_t *space) {
    for (uint64_t i = 256; i < 512; i++) {
        space->pml4->entries[i] = kernel_space.pml4->entries[i];
    }
    return true;
}