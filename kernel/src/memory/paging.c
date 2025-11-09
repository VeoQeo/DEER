#include "include/memory/paging.h"
#include "include/memory/pmm.h"
#include "include/drivers/serial.h"
#include "libc/string.h"
#include "libc/stdio.h"
#include "include/interrupts/isr.h"
#include "include/interrupts/idt.h"
#include <limine.h>

static volatile struct limine_hhdm_response *current_hhdm_response = NULL;

void paging_init(volatile struct limine_hhdm_response *hhdm_response) {
    serial_puts("[PAGING] Initializing paging...\n");
    current_hhdm_response = hhdm_response;
    
    uint64_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    uint64_t cr4;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    
    serial_puts("[PAGING] CR0: 0x");
    char buffer[32];
    serial_puts(itoa(cr0, buffer, 16));
    serial_puts("\n");
    
    serial_puts("[PAGING] CR4: 0x");
    serial_puts(itoa(cr4, buffer, 16));
    serial_puts("\n");
    
    if (cr0 & (1UL << 31)) {
        serial_puts("[PAGING] Paging is already enabled by bootloader\n");
        uint64_t current_cr3 = paging_get_cr3();
        serial_puts("[PAGING] Current CR3: 0x");
        serial_puts(itoa(current_cr3, buffer, 16));
        serial_puts("\n");
        
        if (current_hhdm_response) {
            serial_puts("[PAGING] HHDM offset: 0x");
            serial_puts(itoa(current_hhdm_response->offset, buffer, 16));
            serial_puts("\n");
            
            uint64_t cr3_virtual = current_cr3 + current_hhdm_response->offset;
            serial_puts("[PAGING] CR3 virtual address: 0x");
            serial_puts(itoa(cr3_virtual, buffer, 16));
            serial_puts("\n");
        }
        
        serial_puts("[PAGING] Using existing page tables from bootloader\n");
        return;
    }
    
    if (!current_hhdm_response) {
        serial_puts("[PAGING] ERROR: No HHDM response!\n");
        return;
    }
    
    serial_puts("[PAGING] HHDM offset: 0x");
    serial_puts(itoa(current_hhdm_response->offset, buffer, 16));
    serial_puts("\n");
    
    serial_puts("[PAGING] Paging setup complete\n");
}

void paging_load_cr3(uint64_t cr3_value) {
    asm volatile("mov %0, %%cr3" :: "r"(cr3_value));
}

uint64_t paging_get_cr3(void) {
    uint64_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

void paging_invalidate_tlb(uint64_t virtual_addr) {
    asm volatile("invlpg (%0)" :: "r"(virtual_addr) : "memory");
}

bool paging_map_page(uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags) {
    if (!current_hhdm_response) {
        serial_puts("[PAGING] ERROR: No HHDM response for mapping!\n");
        return false;
    }
    
    uint64_t cr3 = paging_get_cr3();
    uint64_t* pml4 = (uint64_t*)(cr3 + current_hhdm_response->offset);
    
    uint64_t pml4_index = (virtual_addr >> 39) & 0x1FF;
    uint64_t pdp_index = (virtual_addr >> 30) & 0x1FF;
    uint64_t pd_index = (virtual_addr >> 21) & 0x1FF;
    uint64_t pt_index = (virtual_addr >> 12) & 0x1FF;
    
    if (!(pml4[pml4_index] & PAGING_PRESENT)) {
        uint64_t pdp_physical = pmm_alloc_page();
        if (!pdp_physical) {
            serial_puts("[PAGING] ERROR: Failed to allocate PDP table!\n");
            return false;
        }
        
        uint64_t* pdp = (uint64_t*)(pdp_physical + current_hhdm_response->offset);
        memset(pdp, 0, PAGE_SIZE_4K);
        
        pml4[pml4_index] = pdp_physical | PAGING_PRESENT | PAGING_WRITABLE | PAGING_USER;
    }
    
    uint64_t* pdp = (uint64_t*)((pml4[pml4_index] & ~0xFFF) + current_hhdm_response->offset);
    
    if (!(pdp[pdp_index] & PAGING_PRESENT)) {
        uint64_t pd_physical = pmm_alloc_page();
        if (!pd_physical) {
            serial_puts("[PAGING] ERROR: Failed to allocate PD table!\n");
            return false;
        }
        
        uint64_t* pd = (uint64_t*)(pd_physical + current_hhdm_response->offset);
        memset(pd, 0, PAGE_SIZE_4K);
        
        pdp[pdp_index] = pd_physical | PAGING_PRESENT | PAGING_WRITABLE | PAGING_USER;
    }
    
    uint64_t* pd = (uint64_t*)((pdp[pdp_index] & ~0xFFF) + current_hhdm_response->offset);
    
    if (!(pd[pd_index] & PAGING_PRESENT)) {
        uint64_t pt_physical = pmm_alloc_page();
        if (!pt_physical) {
            serial_puts("[PAGING] ERROR: Failed to allocate PT table!\n");
            return false;
        }
        
        uint64_t* pt = (uint64_t*)(pt_physical + current_hhdm_response->offset);
        memset(pt, 0, PAGE_SIZE_4K);
        
        pd[pd_index] = pt_physical | PAGING_PRESENT | PAGING_WRITABLE | PAGING_USER;
    }
    
    uint64_t* pt = (uint64_t*)((pd[pd_index] & ~0xFFF) + current_hhdm_response->offset); 
    pt[pt_index] = physical_addr | flags | PAGING_PRESENT;
    paging_invalidate_tlb(virtual_addr);  
    char buffer[64];
    serial_puts("[PAGING] Mapped virtual 0x");
    serial_puts(itoa(virtual_addr, buffer, 16));
    serial_puts(" to physical 0x");
    serial_puts(itoa(physical_addr, buffer, 16));
    serial_puts("\n");
    
    return true;
}

bool paging_unmap_page(uint64_t virtual_addr) {
    if (!current_hhdm_response) {
        serial_puts("[PAGING] ERROR: No HHDM response for unmapping!\n");
        return false;
    }
    
    uint64_t cr3 = paging_get_cr3();
    uint64_t* pml4 = (uint64_t*)(cr3 + current_hhdm_response->offset);
    uint64_t pml4_index = (virtual_addr >> 39) & 0x1FF;
    uint64_t pdp_index = (virtual_addr >> 30) & 0x1FF;
    uint64_t pd_index = (virtual_addr >> 21) & 0x1FF;
    uint64_t pt_index = (virtual_addr >> 12) & 0x1FF;
    
    if (!(pml4[pml4_index] & PAGING_PRESENT)) {
        serial_puts("[PAGING] ERROR: PML4 entry not present!\n");
        return false;
    }
    
    uint64_t* pdp = (uint64_t*)((pml4[pml4_index] & ~0xFFF) + current_hhdm_response->offset);
    if (!(pdp[pdp_index] & PAGING_PRESENT)) {
        serial_puts("[PAGING] ERROR: PDP entry not present!\n");
        return false;
    }
    
    uint64_t* pd = (uint64_t*)((pdp[pdp_index] & ~0xFFF) + current_hhdm_response->offset);
    if (!(pd[pd_index] & PAGING_PRESENT)) {
        serial_puts("[PAGING] ERROR: PD entry not present!\n");
        return false;
    }
    
    uint64_t* pt = (uint64_t*)((pd[pd_index] & ~0xFFF) + current_hhdm_response->offset);
    if (!(pt[pt_index] & PAGING_PRESENT)) {
        serial_puts("[PAGING] ERROR: Page table entry not present!\n");
        return false;
    }
    
    uint64_t physical_addr = pt[pt_index] & ~0xFFF;
    pmm_free_page(physical_addr);
    
    pt[pt_index] = 0;
    
    paging_invalidate_tlb(virtual_addr);
    
    char buffer[64];
    serial_puts("[PAGING] Unmapped virtual 0x");
    serial_puts(itoa(virtual_addr, buffer, 16));
    serial_puts("\n");
    
    return true;
}

bool paging_is_mapped(uint64_t virtual_addr) {
    if (!current_hhdm_response) return false;
    
    uint64_t cr3 = paging_get_cr3();
    uint64_t* pml4 = (uint64_t*)(cr3 + current_hhdm_response->offset);
    
    uint64_t pml4_index = (virtual_addr >> 39) & 0x1FF;
    if (!(pml4[pml4_index] & PAGING_PRESENT)) return false;
    
    uint64_t* pdp = (uint64_t*)((pml4[pml4_index] & ~0xFFF) + current_hhdm_response->offset);
    uint64_t pdp_index = (virtual_addr >> 30) & 0x1FF;
    if (!(pdp[pdp_index] & PAGING_PRESENT)) return false;
    
    uint64_t* pd = (uint64_t*)((pdp[pdp_index] & ~0xFFF) + current_hhdm_response->offset);
    uint64_t pd_index = (virtual_addr >> 21) & 0x1FF;
    if (!(pd[pd_index] & PAGING_PRESENT)) return false;
    
    uint64_t* pt = (uint64_t*)((pd[pd_index] & ~0xFFF) + current_hhdm_response->offset);
    uint64_t pt_index = (virtual_addr >> 12) & 0x1FF;
    
    return (pt[pt_index] & PAGING_PRESENT) != 0;
}

uint64_t paging_get_physical_address(uint64_t virtual_addr) {
    if (!current_hhdm_response) {
        serial_puts("[PAGING_DEBUG] ERROR: No HHDM response!\n");
        return 0;
    }
    
    char buffer[64];
    serial_puts("[PAGING_DEBUG] Looking up virtual: 0x");
    serial_puts(itoa(virtual_addr, buffer, 16));
    serial_puts("\n");
    
    if (virtual_addr >= current_hhdm_response->offset && 
        virtual_addr < (current_hhdm_response->offset + 0x8000000000)) {
        uint64_t physical = virtual_addr - current_hhdm_response->offset;
        
        serial_puts("[PAGING_DEBUG] HHDM conversion: Virtual 0x");
        serial_puts(itoa(virtual_addr, buffer, 16));
        serial_puts(" -> Physical 0x");
        serial_puts(itoa(physical, buffer, 16));
        serial_puts("\n");
        
        return physical;
    }
    
    serial_puts("[PAGING_DEBUG] Using page table lookup for: 0x");
    serial_puts(itoa(virtual_addr, buffer, 16));
    serial_puts("\n");
    
    uint64_t cr3 = paging_get_cr3();
    uint64_t* pml4 = (uint64_t*)(cr3 + current_hhdm_response->offset);
    
    uint64_t pml4_index = (virtual_addr >> 39) & 0x1FF;
    if (!(pml4[pml4_index] & PAGING_PRESENT)) {
        serial_puts("[PAGING_DEBUG] PML4 entry not present\n");
        return 0;
    }
    
    uint64_t* pdp = (uint64_t*)((pml4[pml4_index] & ~0xFFF) + current_hhdm_response->offset);
    uint64_t pdp_index = (virtual_addr >> 30) & 0x1FF;
    if (!(pdp[pdp_index] & PAGING_PRESENT)) {
        serial_puts("[PAGING_DEBUG] PDP entry not present\n");
        return 0;
    }
    
    uint64_t* pd = (uint64_t*)((pdp[pdp_index] & ~0xFFF) + current_hhdm_response->offset);
    uint64_t pd_index = (virtual_addr >> 21) & 0x1FF;
    if (!(pd[pd_index] & PAGING_PRESENT)) {
        serial_puts("[PAGING_DEBUG] PD entry not present\n");
        return 0;
    }
    
    uint64_t* pt = (uint64_t*)((pd[pd_index] & ~0xFFF) + current_hhdm_response->offset);
    uint64_t pt_index = (virtual_addr >> 12) & 0x1FF;
    
    if (!(pt[pt_index] & PAGING_PRESENT)) {
        serial_puts("[PAGING_DEBUG] Page table entry not present\n");
        return 0;
    }
    
    uint64_t physical = pt[pt_index] & ~0xFFF;
    
    serial_puts("[PAGING_DEBUG] Page table lookup: Virtual 0x");
    serial_puts(itoa(virtual_addr, buffer, 16));
    serial_puts(" -> PTE: 0x");
    serial_puts(itoa(pt[pt_index], buffer, 16));
    serial_puts(" -> Physical: 0x");
    serial_puts(itoa(physical, buffer, 16));
    serial_puts("\n");
    
    return physical;
}

void* paging_physical_to_virtual(uint64_t physical_addr) {
    if (!current_hhdm_response) {
        return (void*)physical_addr;
    }
    
    void* virtual_addr = (void*)(physical_addr + current_hhdm_response->offset);
    
    char buffer[64];
    serial_puts("[PAGING_DEBUG] Physical to virtual: Physical 0x");
    serial_puts(itoa(physical_addr, buffer, 16));
    serial_puts(" -> Virtual 0x");
    serial_puts(itoa((uint64_t)virtual_addr, buffer, 16));
    serial_puts("\n");
    
    return virtual_addr;
}

uint64_t paging_virtual_to_physical(void* virtual_addr) {
    uint64_t vaddr = (uint64_t)virtual_addr;
    
    char buffer[64];
    serial_puts("[PAGING_DEBUG] Virtual to physical: Virtual 0x");
    serial_puts(itoa(vaddr, buffer, 16));
    serial_puts("\n");
    
    if (current_hhdm_response && vaddr >= current_hhdm_response->offset) {
        uint64_t physical = vaddr - current_hhdm_response->offset;
        serial_puts("[PAGING_DEBUG] Using HHDM conversion -> Physical 0x");
        serial_puts(itoa(physical, buffer, 16));
        serial_puts("\n");
        return physical;
    }
    
    uint64_t physical = paging_get_physical_address(vaddr);
    serial_puts("[PAGING_DEBUG] Using page table conversion -> Physical 0x");
    serial_puts(itoa(physical, buffer, 16));
    serial_puts("\n");
    
    return physical;
}

void* paging_hhdm_to_virtual(uint64_t physical_addr) {
    return paging_physical_to_virtual(physical_addr);
}

void handle_page_fault(struct registers *regs) {
    uint64_t fault_address;
    uint64_t error_code = regs->err_code;
    
    asm volatile("mov %%cr2, %0" : "=r"(fault_address));
    
    printf("\n=== PAGE FAULT DETECTED ===\n");
    printf("Fault Address: 0x%x\n", fault_address);
    printf("Error Code: 0x%x\n", error_code);
    
    if (error_code & PF_PRESENT) {
        printf("Type: Protection Violation\n");
        if (error_code & PF_WRITE) {
            printf("Operation: Write to read-only page\n");
        } else {
            printf("Operation: Read from protected page\n");
        }
    } else {
        printf("Type: Page Not Present\n");
        printf("Operation: %s\n", (error_code & PF_WRITE) ? "Write" : "Read");
    }
    
    printf("Access Mode: %s\n", (error_code & PF_USER) ? "User" : "Kernel");
    
    if (!(error_code & PF_PRESENT)) {
        uint64_t page_base = fault_address & ~0xFFF;
    
        if (fault_address >= KERNEL_VIRTUAL_BASE && fault_address < KERNEL_VIRTUAL_BASE + 0x10000000) {
            printf("Attempting to map kernel page at 0x%x...\n", page_base);
            
            uint64_t physical_page = pmm_alloc_page();
            if (physical_page) {
                if (paging_map_page(page_base, physical_page, PAGING_PRESENT | PAGING_WRITABLE)) {
                    printf("Successfully mapped page\n");
                    memset((void*)page_base, 0, PAGE_SIZE_4K); 
                    printf("Page zero-initialized\n");
                    printf("=== PAGE FAULT RESOLVED ===\n\n");
                    return; 
                }
                pmm_free_page(physical_page);
            }
            printf("Failed to map page\n");
        } else {
            printf("Address outside kernel range\n");
        }
    } else {
        printf("Protection violation - cannot handle automatically\n");
    }
    
    printf("\n!!! UNRECOVERABLE PAGE FAULT !!!\n");
    printf("System Halted.\n");
    
    for(;;) {
        asm volatile("hlt");
    }
}

void handle_double_fault(struct registers *regs) {
    printf("\n!!! DOUBLE FAULT DETECTED !!!\n");
    printf("Error Code: 0x%x\n", regs->err_code);
    printf("This is a critical system error.\n");
    printf("Possible causes:\n");
    printf(" - Stack overflow/corruption\n");
    printf(" - Interrupt handler failure\n"); 
    printf(" - Kernel memory corruption\n");
    printf("System Halted.\n");
    
    for(;;) {
        asm volatile("hlt");
    }
}

void handle_general_protection_fault(struct registers *regs) {
    printf("\n!!! GENERAL PROTECTION FAULT !!!\n");
    printf("Error Code: 0x%x\n", regs->err_code);
    printf("Instruction Pointer: 0x%x\n", regs->rip);
    
    if (regs->err_code == 0) {
        printf("Type: Null selector reference\n");
    } else {
        printf("Type: Privilege violation or invalid segment access\n");
    }
    
    if (regs->err_code == 0) {
        printf("Non-critical GPF, attempting to continue...\n");
        return;
    }
    
    printf("Critical GPF - System Halted.\n");
    
    for(;;) {
        asm volatile("hlt");
    }
}

bool paging_map_page_secure(uint64_t virtual_addr, uint64_t physical_addr, 
                           uint64_t flags, bool require_guard_pages) {
    if (virtual_addr == 0 || physical_addr == 0) {
        serial_puts("[PAGING SECURITY] Attempt to map NULL address!\n");
        return false;
    }
    
    if ((virtual_addr >= 0xFFFF800000000000 && virtual_addr < 0xFFFF800000000000 + 0x80000000) &&
        !(flags & PAGING_USER)) {
        serial_puts("[PAGING SECURITY] User mapping in kernel space!\n");
        return false;
    }
    
    if (require_guard_pages) {
        uint64_t guard_addr = virtual_addr - PAGE_SIZE_4K;
        if (!paging_map_page(guard_addr, pmm_alloc_page(), PAGING_NO_EXECUTE)) {
            serial_puts("[PAGING SECURITY] Failed to create guard page!\n");
            return false;
        }
    }
    
    return paging_map_page(virtual_addr, physical_addr, flags);
}

bool paging_validate_integrity(void) {
    if (!current_hhdm_response) return false;
    
    uint64_t cr3 = paging_get_cr3();
    uint64_t* pml4 = (uint64_t*)(cr3 + current_hhdm_response->offset);
    
    if ((pml4[511] & PAGING_PRESENT) == 0) {
        serial_puts("[PAGING SECURITY] PML4 self-reference missing!\n");
        return false;
    }
    
    uint64_t kernel_base = 0xFFFFFFFF80000000;
    uint64_t pml4_index = (kernel_base >> 39) & 0x1FF;
    
    if ((pml4[pml4_index] & PAGING_PRESENT) == 0) {
        serial_puts("[PAGING SECURITY] Kernel space not mapped!\n");
        return false;
    }
    
    return true;
}