#include "include/memory/pmm.h"
#include "include/memory/paging.h"
#include "include/drivers/serial.h"
#include "libc/string.h"
#include "libc/stdio.h"
#include <limine.h>

// Совместимость с разными версиями Limine
#ifndef LIMINE_MEMMAP_EXECUTABLE_AND_MODULES
    #ifdef LIMINE_MEMMAP_KERNEL_AND_MODULES
        #define LIMINE_MEMMAP_EXECUTABLE_AND_MODULES LIMINE_MEMMAP_KERNEL_AND_MODULES
    #else
        #define LIMINE_MEMMAP_EXECUTABLE_AND_MODULES 10  // Стандартное значение
    #endif
#endif

static volatile struct limine_memmap_response *current_memmap_response = NULL;
static volatile struct limine_hhdm_response *current_hhdm_response = NULL;

// Bitmap for tracking physical pages
static uint8_t* bitmap = NULL;
static uint64_t bitmap_size = 0;
static uint64_t total_pages = 0;
static uint64_t used_pages = 0;
static uint64_t total_memory = 0;
static uint64_t bitmap_pages = 0;

// Helper functions
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

static uint64_t find_free_page(void) {
    if (!current_memmap_response) {
        return (uint64_t)-1;
    }

    // Начинаем поиск с первой страницы, пропуская нулевую
    for (uint64_t i = 1; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            // Проверяем, что страница действительно доступна
            bool in_usable_region = false;
            for (uint64_t j = 0; j < current_memmap_response->entry_count; j++) {
                struct limine_memmap_entry* entry = current_memmap_response->entries[j];
                if (entry->type == LIMINE_MEMMAP_USABLE) {
                    uint64_t page_addr = i * PAGE_SIZE_4K;
                    if (page_addr >= entry->base && page_addr < entry->base + entry->length) {
                        in_usable_region = true;
                        break;
                    }
                }
            }
            
            if (in_usable_region) {
                return i;
            }
        }
    }
    return (uint64_t)-1;
}

void pmm_init(volatile struct limine_memmap_response *memmap_response,
              volatile struct limine_hhdm_response *hhdm_response) {
    serial_puts("[PMM] Initializing Physical Memory Manager...\n");
    
    // Сохраняем указатели для использования в других функциях
    current_memmap_response = memmap_response;
    current_hhdm_response = hhdm_response;
    
    if (!current_memmap_response) {
        serial_puts("[PMM] ERROR: No memory map response!\n");
        return;
    }
    
    if (!current_hhdm_response) {
        serial_puts("[PMM] ERROR: No HHDM response!\n");
        return;
    }
    
    // Calculate total memory and find largest usable region for bitmap
    uint64_t highest_address = 0;
    uint64_t largest_region_size = 0;
    uint64_t largest_region_base = 0;
    
    serial_puts("[PMM] Memory map entries:\n");
    
    for (uint64_t i = 0; i < current_memmap_response->entry_count; i++) {
        struct limine_memmap_entry* entry = current_memmap_response->entries[i];
        
        char type_str[32];
        switch (entry->type) {
            case LIMINE_MEMMAP_USABLE:
                strcpy(type_str, "USABLE");
                break;
            case LIMINE_MEMMAP_RESERVED:
                strcpy(type_str, "RESERVED");
                break;
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
                strcpy(type_str, "ACPI_RECLAIMABLE");
                break;
            case LIMINE_MEMMAP_ACPI_NVS:
                strcpy(type_str, "ACPI_NVS");
                break;
            case LIMINE_MEMMAP_BAD_MEMORY:
                strcpy(type_str, "BAD_MEMORY");
                break;
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
                strcpy(type_str, "BOOTLOADER_RECLAIM");
                break;
            case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES:
                strcpy(type_str, "KERNEL");
                break;
            case LIMINE_MEMMAP_FRAMEBUFFER:
                strcpy(type_str, "FRAMEBUFFER");
                break;
            default:
                strcpy(type_str, "UNKNOWN");
                break;
        }
        
        char buffer[64];
        serial_puts("  [");
        serial_puts(itoa(i, buffer, 10));
        serial_puts("] Base: 0x");
        serial_puts(itoa(entry->base, buffer, 16));
        serial_puts(", Length: 0x");
        serial_puts(itoa(entry->length, buffer, 16));
        serial_puts(", Type: ");
        serial_puts(type_str);
        serial_puts("\n");
        
        total_memory += entry->length;
        
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t end = entry->base + entry->length;
            if (end > highest_address) {
                highest_address = end;
            }
            
            // Find largest usable region for bitmap
            if (entry->length > largest_region_size) {
                largest_region_size = entry->length;
                largest_region_base = entry->base;
            }
        }
    }
    
    // Calculate total pages (4KB each)
    total_pages = highest_address / PAGE_SIZE_4K;
    bitmap_size = (total_pages + 7) / 8; // Round up to nearest byte
    bitmap_pages = (bitmap_size + PAGE_SIZE_4K - 1) / PAGE_SIZE_4K;
    
    serial_puts("[PMM] Total memory: ");
    char buffer[32];
    serial_puts(itoa(total_memory / 1024 / 1024, buffer, 10));
    serial_puts(" MB\n");
    
    serial_puts("[PMM] Total pages: ");
    serial_puts(itoa(total_pages, buffer, 10));
    serial_puts(" (");
    serial_puts(itoa(total_pages * PAGE_SIZE_4K / 1024 / 1024, buffer, 10));
    serial_puts(" MB)\n");
    
    serial_puts("[PMM] Bitmap size: ");
    serial_puts(itoa(bitmap_size, buffer, 10));
    serial_puts(" bytes (");
    serial_puts(itoa(bitmap_pages, buffer, 10));
    serial_puts(" pages)\n");
    
    // Find a location for the bitmap in the largest usable region
    if (largest_region_base == 0 || largest_region_size < bitmap_size) {
        serial_puts("[PMM] ERROR: No suitable region for bitmap!\n");
        return;
    }
    
    // Place bitmap at the beginning of the largest region
    uint64_t bitmap_physical = largest_region_base;
    bitmap = (uint8_t*)(bitmap_physical + current_hhdm_response->offset);
    
    // Initialize bitmap (all pages are free initially)
    memset(bitmap, 0, bitmap_size);
    used_pages = 0;
    
    serial_puts("[PMM] Bitmap placed at physical: 0x");
    serial_puts(itoa(bitmap_physical, buffer, 16));
    serial_puts(", virtual: 0x");
    serial_puts(itoa((uint64_t)bitmap, buffer, 16));
    serial_puts("\n");
    
    // Mark all non-usable memory as used
    for (uint64_t i = 0; i < current_memmap_response->entry_count; i++) {
        struct limine_memmap_entry* entry = current_memmap_response->entries[i];
        
        if (entry->type != LIMINE_MEMMAP_USABLE) {
            // Mark this region as used in bitmap
            uint64_t start_page = entry->base / PAGE_SIZE_4K;
            uint64_t end_page = (entry->base + entry->length + PAGE_SIZE_4K - 1) / PAGE_SIZE_4K;
            
            for (uint64_t page = start_page; page < end_page; page++) {
                if (page < total_pages) {
                    bitmap_set(page);
                }
            }
        }
    }
    
    // Mark the bitmap region itself as used
    uint64_t bitmap_start_page = bitmap_physical / PAGE_SIZE_4K;
    uint64_t bitmap_end_page = (bitmap_physical + bitmap_size + PAGE_SIZE_4K - 1) / PAGE_SIZE_4K;
    
    for (uint64_t page = bitmap_start_page; page < bitmap_end_page; page++) {
        if (page < total_pages) {
            bitmap_set(page);
        }
    }
    
    serial_puts("[PMM] Initial memory state: ");
    serial_puts(itoa(used_pages, buffer, 10));
    serial_puts(" pages used, ");
    serial_puts(itoa(total_pages - used_pages, buffer, 10));
    serial_puts(" pages free\n");
    
    serial_puts("[PMM] Physical Memory Manager initialized successfully\n");
}

uint64_t pmm_alloc_page(void) {
    if (!bitmap) {
        serial_puts("[PMM] ERROR: PMM not initialized!\n");
        return 0;
    }
    
    uint64_t page = find_free_page();
    
    if (page == (uint64_t)-1) {
        serial_puts("[PMM] ERROR: Out of physical memory!\n");
        return 0;
    }
    
    bitmap_set(page);
    
    char buffer[32];
    serial_puts("[PMM] Allocated page: 0x");
    serial_puts(itoa(page * PAGE_SIZE_4K, buffer, 16));
    serial_puts(" (");
    serial_puts(itoa(page, buffer, 10));
    serial_puts(")\n");
    
    return page * PAGE_SIZE_4K;
}

void pmm_free_page(uint64_t physical_addr) {
    if (!bitmap) {
        serial_puts("[PMM] ERROR: PMM not initialized!\n");
        return;
    }
    
    uint64_t page = physical_addr / PAGE_SIZE_4K;
    
    if (page >= total_pages) {
        serial_puts("[PMM] ERROR: Attempt to free invalid page!\n");
        return;
    }
    
    if (!bitmap_test(page)) {
        serial_puts("[PMM] WARNING: Page already free!\n");
        return;
    }
    
    bitmap_clear(page);
    
    char buffer[32];
    serial_puts("[PMM] Freed page: 0x");
    serial_puts(itoa(physical_addr, buffer, 16));
    serial_puts(" (");
    serial_puts(itoa(page, buffer, 10));
    serial_puts(")\n");
}

uint64_t pmm_get_free_memory(void) {
    return (total_pages - used_pages) * PAGE_SIZE_4K;
}

uint64_t pmm_get_used_memory(void) {
    return used_pages * PAGE_SIZE_4K;
}

uint64_t pmm_get_total_memory(void) {
    return total_pages * PAGE_SIZE_4K;
}

void pmm_dump_memory_map(void) {
    if (!current_memmap_response) {
        serial_puts("[PMM] ERROR: No memory map available!\n");
        return;
    }
    
    serial_puts("[PMM] Memory Map Dump:\n");
    
    char buffer[64];
    uint64_t total_usable = 0;
    uint64_t total_reserved = 0;
    
    for (uint64_t i = 0; i < current_memmap_response->entry_count; i++) {
        struct limine_memmap_entry* entry = current_memmap_response->entries[i];
        
        char type_str[32];
        switch (entry->type) {
            case LIMINE_MEMMAP_USABLE: 
                strcpy(type_str, "USABLE"); 
                total_usable += entry->length;
                break;
            case LIMINE_MEMMAP_RESERVED: 
                strcpy(type_str, "RESERVED"); 
                total_reserved += entry->length;
                break;
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE: strcpy(type_str, "ACPI_RECLAIM"); break;
            case LIMINE_MEMMAP_ACPI_NVS: strcpy(type_str, "ACPI_NVS"); break;
            case LIMINE_MEMMAP_BAD_MEMORY: strcpy(type_str, "BAD"); break;
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE: strcpy(type_str, "BOOTLOADER"); break;
            case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES: strcpy(type_str, "KERNEL"); break;
            case LIMINE_MEMMAP_FRAMEBUFFER: strcpy(type_str, "FRAMEBUFFER"); break;
            default: strcpy(type_str, "UNKNOWN"); break;
        }
        
        serial_puts("  ");
        serial_puts(type_str);
        serial_puts(": 0x");
        serial_puts(itoa(entry->base, buffer, 16));
        serial_puts(" - 0x");
        serial_puts(itoa(entry->base + entry->length - 1, buffer, 16));
        serial_puts(" (");
        serial_puts(itoa(entry->length / 1024 / 1024, buffer, 10));
        serial_puts(" MB)\n");
    }
    
    // Вывод суммарной информации
    serial_puts("[PMM] Summary: ");
    serial_puts(itoa(total_usable / 1024 / 1024, buffer, 10));
    serial_puts(" MB usable, ");
    serial_puts(itoa(total_reserved / 1024 / 1024, buffer, 10));
    serial_puts(" MB reserved\n");
    
    // Точная статистика PMM
    serial_puts("[PMM] PMM Stats: ");
    serial_puts(itoa(pmm_get_free_memory() / 1024 / 1024, buffer, 10));
    serial_puts(" MB free, ");
    serial_puts(itoa(pmm_get_used_memory() / 1024, buffer, 10));
    serial_puts(" KB used, ");
    serial_puts(itoa(total_pages, buffer, 10));
    serial_puts(" total pages\n");
}