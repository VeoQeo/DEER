#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../interrupts/isr.h"
#include <limine.h>

// Page table entry flags
#define PAGING_PRESENT         (1ULL << 0)
#define PAGING_WRITABLE        (1ULL << 1)
#define PAGING_USER            (1ULL << 2)
#define PAGING_WRITE_THROUGH   (1ULL << 3)
#define PAGING_CACHE_DISABLE   (1ULL << 4)
#define PAGING_ACCESSED        (1ULL << 5)
#define PAGING_DIRTY           (1ULL << 6)
#define PAGING_HUGE_PAGE       (1ULL << 7)
#define PAGING_GLOBAL          (1ULL << 8)
#define PAGING_NO_EXECUTE      (1ULL << 63)
#define PF_PRESENT      (1 << 0)    // Page present
#define PF_WRITE        (1 << 1)    // Write operation
#define PF_USER         (1 << 2)    // User mode access
#define PF_RESERVED     (1 << 3)    // Reserved bit set
#define PF_INSTRUCTION  (1 << 4)    // Instruction fetch

// Page sizes
#define PAGE_SIZE_4K 0x1000
#define PAGE_SIZE_2M 0x200000
#define PAGE_SIZE_1G 0x40000000

// Virtual memory layout
#define KERNEL_VIRTUAL_BASE 0xFFFFFFFF80000000
#define HHDM_OFFSET         0xFFFF800000000000

// Page table structure
typedef uint64_t page_table_entry_t;

struct page_table {
    page_table_entry_t entries[512];
} __attribute__((aligned(PAGE_SIZE_4K)));

// Paging functions
void paging_init(volatile struct limine_hhdm_response *hhdm_response);
void paging_load_cr3(uint64_t cr3_value);
uint64_t paging_get_cr3(void);
void paging_invalidate_tlb(uint64_t virtual_addr);

bool paging_map_page(uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags);
bool paging_unmap_page(uint64_t virtual_addr);
bool paging_is_mapped(uint64_t virtual_addr);


// Memory conversion functions
void* paging_physical_to_virtual(uint64_t physical_addr);
uint64_t paging_virtual_to_physical(void* virtual_addr);
void* paging_hhdm_to_virtual(uint64_t physical_addr);
uint64_t paging_get_physical_address(uint64_t virtual_addr);

void handle_page_fault(struct registers *regs);
void handle_double_fault(struct registers *regs);
void handle_general_protection_fault(struct registers *regs);

#endif // PAGING_H