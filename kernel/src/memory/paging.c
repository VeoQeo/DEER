#include "include/memory/paging.h"
#include "include/memory/pmm.h"
#include "include/memory/heap.h"
#include "include/drivers/serial.h"
#include "libc/string.h"
#include "libc/stdio.h"
#include "include/interrupts/isr.h"

static volatile struct limine_hhdm_response *current_hhdm_response = NULL;

void paging_init(volatile struct limine_hhdm_response *hhdm_response) {
    serial_puts("[PAGING] Initializing...\n");
    
    current_hhdm_response = hhdm_response;
    
    if (!current_hhdm_response) {
        serial_puts("[PAGING] ERROR: No HHDM response!\n");
        return;
    }
    
    uint64_t cr0, cr4, cr3;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    
    char buffer[32];
    serial_puts("[PAGING] CR0: 0x");
    serial_puts(itoa(cr0, buffer, 16));
    serial_puts(", CR4: 0x");
    serial_puts(itoa(cr4, buffer, 16));
    serial_puts(", CR3: 0x");
    serial_puts(itoa(cr3, buffer, 16));
    serial_puts("\n");
    
    serial_puts("[PAGING] HHDM offset: 0x");
    serial_puts(itoa(current_hhdm_response->offset, buffer, 16));
    serial_puts("\n");
    
    if (cr0 & (1UL << 31)) {
        serial_puts("[PAGING] Paging already enabled by bootloader\n");
    }
    
    serial_puts("[PAGING] Ready\n");
}

void paging_load_cr3(uint64_t cr3_value) {
    asm volatile("mov %0, %%cr3" :: "r"(cr3_value));
}

// handle double fault
void handle_double_fault(struct registers *regs) {
    serial_puts("\n[EXCEPTION] DOUBLE FAULT! System halted.\n");
    
    for (;;);
}

uint64_t paging_get_cr3(void) {
    uint64_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

void paging_invalidate_tlb(uint64_t virtual_addr) {
    asm volatile("invlpg (%0)" :: "r"(virtual_addr) : "memory");
}

static page_table_t* get_next_table(page_table_t* current, uint64_t index, bool create) {
    if (current->entries[index] & PAGING_PRESENT) {
        uint64_t phys = current->entries[index] & ~0xFFF;
        return (page_table_t*)(phys + current_hhdm_response->offset);
    }
    
    if (!create) return NULL;
    
    uint64_t phys = pmm_alloc_page();
    if (!phys) return NULL;
    
    page_table_t* next = (page_table_t*)(phys + current_hhdm_response->offset);
    memset(next, 0, PAGE_SIZE_4K);
    
    current->entries[index] = phys | PAGING_PRESENT | PAGING_WRITABLE;
    return next;
}

bool paging_map_page(uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags) {
    if (!current_hhdm_response) return false;
    
    uint64_t cr3 = paging_get_cr3();
    page_table_t* pml4 = (page_table_t*)(cr3 + current_hhdm_response->offset);
    
    uint64_t pml4_index = (virtual_addr >> 39) & 0x1FF;
    uint64_t pdp_index = (virtual_addr >> 30) & 0x1FF;
    uint64_t pd_index = (virtual_addr >> 21) & 0x1FF;
    uint64_t pt_index = (virtual_addr >> 12) & 0x1FF;
    
    page_table_t* pdp = get_next_table(pml4, pml4_index, true);
    if (!pdp) return false;
    
    page_table_t* pd = get_next_table(pdp, pdp_index, true);
    if (!pd) return false;
    
    page_table_t* pt = get_next_table(pd, pd_index, true);
    if (!pt) return false;
    
    pt->entries[pt_index] = physical_addr | flags | PAGING_PRESENT;
    paging_invalidate_tlb(virtual_addr);
    
    return true;
}

bool paging_unmap_page(uint64_t virtual_addr) {
    if (!current_hhdm_response) return false;
    
    uint64_t cr3 = paging_get_cr3();
    page_table_t* pml4 = (page_table_t*)(cr3 + current_hhdm_response->offset);
    
    uint64_t pml4_index = (virtual_addr >> 39) & 0x1FF;
    uint64_t pdp_index = (virtual_addr >> 30) & 0x1FF;
    uint64_t pd_index = (virtual_addr >> 21) & 0x1FF;
    uint64_t pt_index = (virtual_addr >> 12) & 0x1FF;
    
    page_table_t* pdp = get_next_table(pml4, pml4_index, false);
    if (!pdp) return false;
    
    page_table_t* pd = get_next_table(pdp, pdp_index, false);
    if (!pd) return false;
    
    page_table_t* pt = get_next_table(pd, pd_index, false);
    if (!pt) return false;
    
    if (!(pt->entries[pt_index] & PAGING_PRESENT)) return false;
    
    pmm_free_page(pt->entries[pt_index] & ~0xFFF);
    pt->entries[pt_index] = 0;
    paging_invalidate_tlb(virtual_addr);
    
    return true;
}

bool paging_is_mapped(uint64_t virtual_addr) {
    if (!current_hhdm_response) return false;
    
    uint64_t cr3 = paging_get_cr3();
    page_table_t* pml4 = (page_table_t*)(cr3 + current_hhdm_response->offset);
    
    uint64_t pml4_index = (virtual_addr >> 39) & 0x1FF;
    if (!(pml4->entries[pml4_index] & PAGING_PRESENT)) return false;
    
    page_table_t* pdp = get_next_table(pml4, pml4_index, false);
    if (!pdp) return false;
    
    uint64_t pdp_index = (virtual_addr >> 30) & 0x1FF;
    if (!(pdp->entries[pdp_index] & PAGING_PRESENT)) return false;
    
    page_table_t* pd = get_next_table(pdp, pdp_index, false);
    if (!pd) return false;
    
    uint64_t pd_index = (virtual_addr >> 21) & 0x1FF;
    if (!(pd->entries[pd_index] & PAGING_PRESENT)) return false;
    
    page_table_t* pt = get_next_table(pd, pd_index, false);
    if (!pt) return false;
    
    uint64_t pt_index = (virtual_addr >> 12) & 0x1FF;
    return (pt->entries[pt_index] & PAGING_PRESENT) != 0;
}

uint64_t paging_get_physical_address(uint64_t virtual_addr) {
    if (!current_hhdm_response) return 0;
    
    if (virtual_addr >= current_hhdm_response->offset) {
        return virtual_addr - current_hhdm_response->offset;
    }
    
    uint64_t cr3 = paging_get_cr3();
    page_table_t* pml4 = (page_table_t*)(cr3 + current_hhdm_response->offset);
    
    uint64_t pml4_index = (virtual_addr >> 39) & 0x1FF;
    if (!(pml4->entries[pml4_index] & PAGING_PRESENT)) return 0;
    
    page_table_t* pdp = get_next_table(pml4, pml4_index, false);
    if (!pdp) return 0;
    
    uint64_t pdp_index = (virtual_addr >> 30) & 0x1FF;
    if (!(pdp->entries[pdp_index] & PAGING_PRESENT)) return 0;
    
    page_table_t* pd = get_next_table(pdp, pdp_index, false);
    if (!pd) return 0;
    
    uint64_t pd_index = (virtual_addr >> 21) & 0x1FF;
    if (!(pd->entries[pd_index] & PAGING_PRESENT)) return 0;
    
    page_table_t* pt = get_next_table(pd, pd_index, false);
    if (!pt) return 0;
    
    uint64_t pt_index = (virtual_addr >> 12) & 0x1FF;
    if (!(pt->entries[pt_index] & PAGING_PRESENT)) return 0;
    
    return pt->entries[pt_index] & ~0xFFF;
}

void* paging_physical_to_virtual(uint64_t physical_addr) {
    if (!current_hhdm_response) {
        return (void*)physical_addr;
    }
    return (void*)(physical_addr + current_hhdm_response->offset);
}

uint64_t paging_virtual_to_physical(void* virtual_addr) {
    return paging_get_physical_address((uint64_t)virtual_addr);
}

void handle_page_fault(struct registers *regs) {
    (void)regs; 
    uint64_t fault_address;
    uint64_t error_code = regs->err_code;
    
    asm volatile("mov %%cr2, %0" : "=r"(fault_address));
    
    if (!(error_code & 0x1)) { 
        if (fault_address >= HEAP_START && fault_address < HEAP_START + HEAP_MAX_SIZE) {
            uint64_t page_base = fault_address & ~0xFFF;
            uint64_t phys = pmm_alloc_page();
            if (phys && paging_map_page(page_base, phys, PAGING_PRESENT | PAGING_WRITABLE)) {
                serial_puts("[PAGING] Auto-mapped heap page at 0x");
                char buf[32];
                serial_puts(itoa(page_base, buf, 16));
                serial_puts("\n");
                memset((void*)page_base, 0, PAGE_SIZE_4K);
                return;
            }
        }
    }
    
    printf("\nPAGE FAULT\n");
    printf("Fault Address: 0x%x\n", fault_address);
    printf("Error Code: 0x%x\n", error_code);
    
    if (error_code & 0x1) {
        printf("Type: Protection Violation\n");
    } else {
        printf("Type: Page Not Present\n");
    }
    
    printf("Unrecoverable page fault - System Halted\n");
    for(;;) asm volatile("hlt");
}

void handle_general_protection_fault(struct registers *regs) {
    printf("\n!!! GENERAL PROTECTION FAULT !!!\n");
    printf("Error Code: 0x%x\n", regs->err_code);
    
    if (regs->err_code == 0) {
        printf("Non-critical GPF - continuing...\n");
        return;
    }
    
    printf("Critical GPF - System Halted\n");
    for(;;) asm volatile("hlt");
}