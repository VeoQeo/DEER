#include "include/memory/pmm.h"
#include "include/drivers/serial.h"
#include "libc/string.h"
#include "libc/stdio.h"

static volatile struct limine_memmap_response *current_memmap = NULL;
static volatile struct limine_hhdm_response *current_hhdm = NULL;

static uint8_t* bitmap = NULL;
static uint64_t bitmap_size = 0;
static uint64_t total_pages = 0;
static uint64_t used_pages = 0;
static uint64_t total_memory = 0;

static void bitmap_set(uint64_t bit) {
    uint64_t byte = bit / 8;
    uint64_t offset = bit % 8;
    bitmap[byte] |= (1 << offset);
    used_pages++;
}

static void bitmap_clear(uint64_t bit) {
    uint64_t byte = bit / 8;
    uint64_t offset = bit % 8;
    bitmap[byte] &= ~(1 << offset);
    used_pages--;
}

static bool bitmap_test(uint64_t bit) {
    uint64_t byte = bit / 8;
    uint64_t offset = bit % 8;
    return (bitmap[byte] & (1 << offset)) != 0;
}

static uint64_t find_free_pages(size_t count) {
    uint64_t consecutive = 0;
    uint64_t start_page = 0;
    
    for (uint64_t i = 1; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            bool usable = false;
            for (uint64_t j = 0; j < current_memmap->entry_count; j++) {
                struct limine_memmap_entry* entry = current_memmap->entries[j];
                if (entry->type == LIMINE_MEMMAP_USABLE) {
                    uint64_t page_addr = i * PAGE_SIZE;
                    if (page_addr >= entry->base && page_addr < entry->base + entry->length) {
                        usable = true;
                        break;
                    }
                }
            }
            
            if (usable) {
                if (consecutive == 0) start_page = i;
                consecutive++;
                if (consecutive == count) return start_page;
            } else {
                consecutive = 0;
            }
        } else {
            consecutive = 0;
        }
    }
    return (uint64_t)-1;
}

void pmm_init(volatile struct limine_memmap_response *memmap_response,
              volatile struct limine_hhdm_response *hhdm_response) {
    serial_puts("[PMM] Initializing...\n");
    
    current_memmap = memmap_response;
    current_hhdm = hhdm_response;
    
    if (!current_memmap || !current_hhdm) {
        serial_puts("[PMM] ERROR: No memory map or HHDM!\n");
        return;
    }
    
    uint64_t highest_addr = 0;
    uint64_t largest_base = 0;
    uint64_t largest_size = 0;

    for (uint64_t i = 0; i < current_memmap->entry_count; i++) {
        struct limine_memmap_entry* entry = current_memmap->entries[i];
        total_memory += entry->length;
        
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t end = entry->base + entry->length;
            if (end > highest_addr) highest_addr = end;
            if (entry->length > largest_size) {
                largest_size = entry->length;
                largest_base = entry->base;
            }
        }
    }
    
    total_pages = highest_addr / PAGE_SIZE;
    bitmap_size = (total_pages + 7) / 8;
    
    bitmap = (uint8_t*)(largest_base + current_hhdm->offset);
    memset(bitmap, 0, bitmap_size);

    for (uint64_t i = 0; i < current_memmap->entry_count; i++) {
        struct limine_memmap_entry* entry = current_memmap->entries[i];
        if (entry->type != LIMINE_MEMMAP_USABLE) {
            uint64_t start = entry->base / PAGE_SIZE;
            uint64_t end = (entry->base + entry->length + PAGE_SIZE - 1) / PAGE_SIZE;
            for (uint64_t page = start; page < end; page++) {
                if (page < total_pages) bitmap_set(page);
            }
        }
    }
    
    uint64_t bitmap_start = largest_base / PAGE_SIZE;
    uint64_t bitmap_end = (largest_base + bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t page = bitmap_start; page < bitmap_end; page++) {
        if (page < total_pages) bitmap_set(page);
    }
    
    serial_puts("[PMM] Ready: ");
    char buf[32];
    serial_puts(itoa(total_pages - used_pages, buf, 10));
    serial_puts(" free pages\n");
}

uint64_t pmm_alloc_page(void) {
    return pmm_alloc_pages(1);
}

uint64_t pmm_alloc_pages(size_t count) {
    if (!bitmap) return 0;
    
    uint64_t start = find_free_pages(count);
    if (start == (uint64_t)-1) {
        serial_puts("[PMM] Out of memory!\n");
        return 0;
    }
    
    for (uint64_t i = 0; i < count; i++) {
        bitmap_set(start + i);
    }
    
    return start * PAGE_SIZE;
}

void pmm_free_page(uint64_t page) {
    pmm_free_pages(page, 1);
}

void pmm_free_pages(uint64_t page, size_t count) {
    if (!bitmap) return;
    
    uint64_t start = page / PAGE_SIZE;
    for (uint64_t i = 0; i < count; i++) {
        if (start + i < total_pages) {
            bitmap_clear(start + i);
        }
    }
}

uint64_t pmm_get_total_memory(void) {
    return total_pages * PAGE_SIZE;
}

uint64_t pmm_get_free_memory(void) {
    return (total_pages - used_pages) * PAGE_SIZE;
}

uint64_t pmm_get_used_memory(void) {
    return used_pages * PAGE_SIZE;
}

void pmm_dump_memory_map(void) {
    if (!current_memmap) return;
    
    serial_puts("[PMM] Memory Map:\n");
    for (uint64_t i = 0; i < current_memmap->entry_count; i++) {
        struct limine_memmap_entry* entry = current_memmap->entries[i];
        char buf[64];
        serial_puts("  Base: 0x");
        serial_puts(itoa(entry->base, buf, 16));
        serial_puts(" Size: ");
        serial_puts(itoa(entry->length / 1024 / 1024, buf, 10));
        serial_puts(" MB\n");
    }
}