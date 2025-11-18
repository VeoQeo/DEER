#include "include/memory/heap.h"
#include "include/memory/paging.h"
#include "include/memory/pmm.h"
#include "include/drivers/serial.h"
#include "libc/string.h"
#include "libc/stdio.h"

static heap_t kernel_heap = {0};

void heap_init(void) {
    serial_puts("[HEAP] Initializing...\n");
    
    size_t initial_pages = HEAP_INITIAL_SIZE / PAGE_SIZE_4K;
    
    for (size_t i = 0; i < initial_pages; i++) {
        uint64_t physical_page = pmm_alloc_page();
        if (!physical_page) {
            serial_puts("[HEAP] ERROR: Failed to allocate physical pages!\n");
            return;
        }
        
        uint64_t virtual_addr = HEAP_START + (i * PAGE_SIZE_4K);
        if (!paging_map_page(virtual_addr, physical_page, 
                           PAGING_PRESENT | PAGING_WRITABLE)) {
            serial_puts("[HEAP] ERROR: Failed to map heap page!\n");
            pmm_free_page(physical_page);
            return;
        }
    }
    
    kernel_heap.start = (void*)HEAP_START;
    kernel_heap.end = (void*)(HEAP_START + HEAP_INITIAL_SIZE);
    kernel_heap.total_size = HEAP_INITIAL_SIZE;
    kernel_heap.used_size = 0;
    kernel_heap.block_count = 1;
    
    heap_block_t* first_block = (heap_block_t*)HEAP_START;
    first_block->size = HEAP_INITIAL_SIZE - sizeof(heap_block_t);
    first_block->used = false;
    first_block->next = NULL;
    first_block->prev = NULL;
    
    kernel_heap.first_block = first_block;
    
    serial_puts("[HEAP] Ready at 0x");
    char buf[32];
    serial_puts(itoa(HEAP_START, buf, 16));
    serial_puts(" (");
    serial_puts(itoa(HEAP_INITIAL_SIZE / 1024, buf, 10));
    serial_puts(" KB)\n");
}

static heap_block_t* find_free_block(size_t size) {
    heap_block_t* current = kernel_heap.first_block;
    heap_block_t* best = NULL;
    
    while (current != NULL) {
        if (!current->used && current->size >= size) {
            if (!best || current->size < best->size) {
                best = current;
            }
        }
        current = current->next;
    }
    return best;
}

void* kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    size = (size + HEAP_ALIGNMENT - 1) & ~(HEAP_ALIGNMENT - 1);
    if (size < sizeof(heap_block_t)) {
        size = sizeof(heap_block_t);
    }
    
    heap_block_t* block = find_free_block(size);
    if (!block) {
        serial_puts("[HEAP] No free block found for size ");
        char buf[32];
        serial_puts(itoa(size, buf, 10));
        serial_puts("\n");
        return NULL;
    }
    
    if (block->size >= size + sizeof(heap_block_t) + HEAP_ALIGNMENT) {
        heap_block_t* new_block = (heap_block_t*)((uint8_t*)block + sizeof(heap_block_t) + size);
        new_block->size = block->size - size - sizeof(heap_block_t);
        new_block->used = false;
        new_block->next = block->next;
        new_block->prev = block;
        
        if (block->next) {
            block->next->prev = new_block;
        }
        block->next = new_block;
        block->size = size;
        
        kernel_heap.block_count++;
    }
    
    block->used = true;
    kernel_heap.used_size += block->size; 
    
    serial_puts("[HEAP] Allocated ");
    char buf[32];
    serial_puts(itoa(size, buf, 10));
    serial_puts(" bytes, total used: ");
    serial_puts(itoa(kernel_heap.used_size, buf, 10));
    serial_puts(" bytes\n");
    
    return (void*)((uint8_t*)block + sizeof(heap_block_t));
}

void kfree(void* ptr) {
    if (!ptr) return;
    
    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    if (!block->used) {
        serial_puts("[HEAP] Double free detected at 0x");
        char buf[32];
        serial_puts(itoa((uint64_t)ptr, buf, 16));
        serial_puts("\n");
        return;
    }
    
    block->used = false;
    kernel_heap.used_size -= block->size;
    
    serial_puts("[HEAP] Freed ");
    char buf[32];
    serial_puts(itoa(block->size, buf, 10));
    serial_puts(" bytes, total used: ");
    serial_puts(itoa(kernel_heap.used_size, buf, 10));
    serial_puts(" bytes\n");
    
    if (block->prev && !block->prev->used) {
        block->prev->size += block->size + sizeof(heap_block_t);
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
        kernel_heap.block_count--;
        block = block->prev;
    }
    
    if (block->next && !block->next->used) {
        block->size += block->next->size + sizeof(heap_block_t);
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
        kernel_heap.block_count--;
    }
}

void* kcalloc(size_t num, size_t size) {
    size_t total = num * size;
    void* ptr = kmalloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void* krealloc(void* ptr, size_t size) {
    if (!ptr) return kmalloc(size);
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    if (size <= block->size) {
        return ptr; 
    }
    
    void* new_ptr = kmalloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, block->size);
        kfree(ptr);
    }
    return new_ptr;
}

size_t heap_get_total_size(void) { 
    return kernel_heap.total_size; 
}

size_t heap_get_used_size(void) { 
    return kernel_heap.used_size; 
}

size_t heap_get_free_size(void) { 
    return kernel_heap.total_size - kernel_heap.used_size; 
}

void heap_dump_blocks(void) {
    serial_puts("[HEAP] Block dump:\n");
    heap_block_t* current = kernel_heap.first_block;
    size_t i = 0;
    size_t total_used = 0;
    size_t total_free = 0;
    
    while (current) {
        char buf[64];
        serial_puts("  ");
        serial_puts(itoa(i++, buf, 10));
        serial_puts(": Addr=0x");
        serial_puts(itoa((uint64_t)current, buf, 16));
        serial_puts(" Size=");
        serial_puts(itoa(current->size, buf, 10));
        serial_puts(" Used=");
        serial_puts(current->used ? "yes" : "no");
        serial_puts("\n");
        
        if (current->used) {
            total_used += current->size;
        } else {
            total_free += current->size;
        }
        
        current = current->next;
    }
    
    serial_puts("[HEAP] Summary: ");
    char buf[32];
    serial_puts(itoa(total_used, buf, 10));
    serial_puts(" bytes used, ");
    serial_puts(itoa(total_free, buf, 10));
    serial_puts(" bytes free, ");
    serial_puts(itoa(kernel_heap.block_count, buf, 10));
    serial_puts(" blocks total\n");
}