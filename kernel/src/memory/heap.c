#include "include/memory/heap.h"
#include "include/memory/paging.h"
#include "include/memory/pmm.h"
#include "include/drivers/serial.h"
#include "libc/string.h"
#include "libc/stdio.h"

static struct heap kernel_heap = {0};
static bool poisoning_enabled = true;
static bool guard_pages_enabled = false;
static bool lockdown_mode = false;

static uint32_t calculate_checksum(struct heap_block* block) {
    uint32_t sum = 0;
    uint8_t* data = (uint8_t*)block;
    
    for (size_t i = 0; i < offsetof(struct heap_block, checksum); i++) {
        sum = (sum << 3) ^ data[i];
    }
    for (size_t i = offsetof(struct heap_block, checksum) + sizeof(uint32_t); 
         i < sizeof(struct heap_block); i++) {
        sum = (sum << 3) ^ data[i];
    }
    
    return sum;
}

bool heap_validate_block(struct heap_block* block) {
    if (!block) return false;
    
    if (block->canary_start != HEAP_CANARY_VALUE || 
        block->canary_end != HEAP_CANARY_VALUE) {
        kernel_heap.stats.buffer_overflow_detected++;
        serial_puts("[HEAP SECURITY] Buffer overflow detected! Canary corrupted.\n");
        return false;
    }
    
    uint32_t saved_checksum = block->checksum;
    block->checksum = 0;
    uint32_t calculated_checksum = calculate_checksum(block);
    block->checksum = saved_checksum;
    
    if (saved_checksum != calculated_checksum) {
        kernel_heap.stats.corruption_count++;
        serial_puts("[HEAP SECURITY] Heap block checksum mismatch! Memory corrupted.\n");
        return false;
    }
    
    if (block->size > HEAP_MAX_ALLOC_SIZE || block->size < HEAP_MIN_ALLOC_SIZE) {
        serial_puts("[HEAP SECURITY] Invalid block size detected!\n");
        return false;
    }
    
    uint64_t block_addr = (uint64_t)block;
    if (block_addr < (uint64_t)kernel_heap.start || 
        block_addr >= (uint64_t)kernel_heap.end) {
        serial_puts("[HEAP SECURITY] Block outside heap boundaries!\n");
        return false;
    }
    
    return true;
}

static void initialize_block(struct heap_block* block, size_t size, size_t requested_size, 
                           bool used, uint8_t protection_flags) {
    block->canary_start = HEAP_CANARY_VALUE;
    block->size = size;
    block->requested_size = requested_size;
    block->used = used;
    block->protection_flags = protection_flags;
    block->next = NULL;
    block->prev = NULL;
    block->allocation_ptr = (void*)((uint8_t*)block + sizeof(struct heap_block));
    block->canary_end = HEAP_CANARY_VALUE;
    
    block->checksum = 0;
    block->checksum = calculate_checksum(block);
    
    if (poisoning_enabled) {
        uint8_t poison = used ? HEAP_ALLOC_POISON : HEAP_FREE_POISON;
        memset(block->allocation_ptr, poison, size);
    }
}

static struct heap_block* find_best_fit_block(size_t size) {
    struct heap_block* current = kernel_heap.first_block;
    struct heap_block* best_fit = NULL;
    
    while (current != NULL) {
        if (!heap_validate_block(current)) {
            serial_puts("[HEAP SECURITY] Corrupted block detected during allocation!\n");
            return NULL;
        }
        
        if (!current->used && current->size >= size) {
            if (best_fit == NULL || current->size < best_fit->size) {
                best_fit = current;
            }
        }
        current = current->next;
    }
    
    return best_fit;
}

void heap_emergency_lockdown(void) {
    lockdown_mode = true;
    serial_puts("[HEAP SECURITY] EMERGENCY LOCKDOWN ACTIVATED!\n");
    serial_puts("[HEAP SECURITY] All further allocations are blocked.\n");
    
    heap_dump_blocks();
    heap_print_stats();
}

void heap_init(void) {
    if (lockdown_mode) {
        serial_puts("[HEAP SECURITY] Cannot initialize - system in lockdown mode!\n");
        return;
    }
    
    serial_puts("[HEAP] Initializing secure kernel heap...\n");
    
    memset(&kernel_heap.stats, 0, sizeof(kernel_heap.stats));
    
    size_t initial_pages = HEAP_INITIAL_SIZE / PAGE_SIZE_4K;
    if (guard_pages_enabled) {
        initial_pages++; 
    }
    
    for (size_t i = 0; i < initial_pages; i++) {
        uint64_t physical_page = pmm_alloc_page();
        if (!physical_page) {
            serial_puts("[HEAP] ERROR: Failed to allocate initial heap pages!\n");
            heap_emergency_lockdown();
            return;
        }
        
        uint64_t virtual_addr = HEAP_START + (i * PAGE_SIZE_4K);
        uint64_t flags = PAGING_PRESENT | PAGING_WRITABLE;
        
        if (guard_pages_enabled && i == initial_pages - 1) {
            flags = PAGING_NO_EXECUTE; 
        }
        
        if (!paging_map_page(virtual_addr, physical_page, flags)) {
            serial_puts("[HEAP] ERROR: Failed to map heap page!\n");
            pmm_free_page(physical_page);
            heap_emergency_lockdown();
            return;
        }
    }
    
    kernel_heap.start = (void*)HEAP_START;
    kernel_heap.end = (void*)(HEAP_START + HEAP_INITIAL_SIZE);
    kernel_heap.total_size = HEAP_INITIAL_SIZE;
    kernel_heap.used_size = sizeof(struct heap_block);
    kernel_heap.alloc_count = 0;
    kernel_heap.free_count = 0;
    kernel_heap.corruption_count = 0;
    
    struct heap_block* first_block = (struct heap_block*)HEAP_START;
    size_t first_block_size = HEAP_INITIAL_SIZE - sizeof(struct heap_block);
    initialize_block(first_block, first_block_size, 0, false, 0);
    
    kernel_heap.first_block = first_block;
    
    serial_puts("[HEAP] Secure kernel heap initialized with canaries and checksums\n");
}

void* kmalloc(size_t size) {
    return kmalloc_protected(size, 0);
}

void* kmalloc_protected(size_t size, uint8_t protection_flags) {
    if (lockdown_mode) {
        serial_puts("[HEAP SECURITY] Allocation blocked - system in lockdown!\n");
        kernel_heap.stats.failed_allocations++;
        return NULL;
    }
    
    if (size == 0 || size > HEAP_MAX_ALLOC_SIZE) {
        serial_puts("[HEAP SECURITY] Invalid allocation size requested!\n");
        kernel_heap.stats.failed_allocations++;
        return NULL;
    }
    
    size = (size + HEAP_ALIGNMENT - 1) & ~(HEAP_ALIGNMENT - 1);
    if (size < HEAP_MIN_ALLOC_SIZE) {
        size = HEAP_MIN_ALLOC_SIZE;
    }
    
    if (!heap_validate_all_blocks()) {
        serial_puts("[HEAP SECURITY] Heap corruption detected before allocation!\n");
        heap_emergency_lockdown();
        return NULL;
    }
    
    struct heap_block* best_fit = find_best_fit_block(size);
    
    if (best_fit == NULL) {
        kernel_heap.stats.failed_allocations++;
        serial_puts("[HEAP] No suitable block found, allocation failed\n");
        return NULL;
    }
    
    if (best_fit->size >= size + sizeof(struct heap_block) + HEAP_MIN_ALLOC_SIZE) {
        struct heap_block* new_block = (struct heap_block*)(
            (uint8_t*)best_fit + sizeof(struct heap_block) + size);
        
        size_t new_block_size = best_fit->size - size - sizeof(struct heap_block);
        initialize_block(new_block, new_block_size, 0, false, 0);
        
        new_block->next = best_fit->next;
        new_block->prev = best_fit;
        
        if (best_fit->next != NULL) {
            best_fit->next->prev = new_block;
        }
        
        best_fit->size = size;
        best_fit->next = new_block;
    }
    
    best_fit->used = true;
    best_fit->protection_flags = protection_flags;
    best_fit->requested_size = size;
    
    best_fit->checksum = 0;
    best_fit->checksum = calculate_checksum(best_fit);
    
    kernel_heap.used_size += best_fit->size + sizeof(struct heap_block);
    kernel_heap.alloc_count++;
    kernel_heap.stats.total_allocations++;
    
    if (poisoning_enabled) {
        memset(best_fit->allocation_ptr, HEAP_ALLOC_POISON, best_fit->size);
    }
    
    if (!heap_validate_block(best_fit)) {
        serial_puts("[HEAP SECURITY] Allocation created corrupted block!\n");
        kfree(best_fit->allocation_ptr); 
        return NULL;
    }
    
    return best_fit->allocation_ptr;
}

void kfree(void* ptr) {
    if (lockdown_mode) {
        serial_puts("[HEAP SECURITY] Free operation blocked - system in lockdown!\n");
        return;
    }
    
    if (ptr == NULL) {
        return;
    }
    
    struct heap_block* block = (struct heap_block*)((uint8_t*)ptr - sizeof(struct heap_block));
    
    if (!heap_is_valid_pointer(ptr)) {
        serial_puts("[HEAP SECURITY] Invalid pointer passed to kfree!\n");
        kernel_heap.stats.failed_allocations++;
        return;
    }
    
    if (!heap_validate_block(block)) {
        serial_puts("[HEAP SECURITY] Cannot free corrupted block!\n");
        heap_emergency_lockdown();
        return;
    }
    
    if (!block->used) {
        serial_puts("[HEAP SECURITY] Double free detected!\n");
        kernel_heap.stats.double_free_attempts++;
        heap_emergency_lockdown();
        return;
    }
    
    block->used = false;
    block->protection_flags = 0;
    kernel_heap.used_size -= block->size + sizeof(struct heap_block);
    kernel_heap.free_count++;
    kernel_heap.stats.total_frees++;
    
    if (poisoning_enabled) {
        memset(block->allocation_ptr, HEAP_FREE_POISON, block->size);
    }
    
    block->checksum = 0;
    block->checksum = calculate_checksum(block);
    
    if (block->prev != NULL && !block->prev->used && heap_validate_block(block->prev)) {
        block->prev->size += block->size + sizeof(struct heap_block);
        block->prev->next = block->next;
        
        if (block->next != NULL) {
            block->next->prev = block->prev;
        }
        block->prev->checksum = 0;
        block->prev->checksum = calculate_checksum(block->prev);  
        block = block->prev;
    }
    
    if (block->next != NULL && !block->next->used && heap_validate_block(block->next)) {
        block->size += block->next->size + sizeof(struct heap_block);
        block->next = block->next->next;
        
        if (block->next != NULL) {
            block->next->prev = block;
        }
        
        block->checksum = 0;
        block->checksum = calculate_checksum(block);
    }
}

bool heap_validate_all_blocks(void) {
    struct heap_block* current = kernel_heap.first_block;
    bool all_valid = true;
    
    while (current != NULL) {
        if (!heap_validate_block(current)) {
            all_valid = false;
            kernel_heap.corruption_count++;
        }
        current = current->next;
    }
    
    if (!all_valid) {
        serial_puts("[HEAP SECURITY] Heap validation failed!\n");
    }
    
    return all_valid;
}

bool heap_is_valid_pointer(void* ptr) {
    if (ptr == NULL) return false;
    
    uint64_t addr = (uint64_t)ptr;
    if (addr < (uint64_t)kernel_heap.start || addr >= (uint64_t)kernel_heap.end) {
        return false;
    }
    
    struct heap_block* block = (struct heap_block*)((uint8_t*)ptr - sizeof(struct heap_block));
    if ((uint64_t)block < (uint64_t)kernel_heap.start || 
        (uint64_t)block >= (uint64_t)kernel_heap.end) {
        return false;
    }
    
    return heap_validate_block(block);
}

void* kcalloc(size_t num, size_t size) {
    size_t total_size = num * size;
    void* ptr = kmalloc(total_size);
    if (ptr != NULL) {
        memset(ptr, 0, total_size); 
    }
    return ptr;
}

void* krealloc(void* ptr, size_t size) {
    if (ptr == NULL) return kmalloc(size);
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    if (!heap_is_valid_pointer(ptr)) {
        serial_puts("[HEAP SECURITY] Invalid pointer in krealloc!\n");
        return NULL;
    }
    
    struct heap_block* block = (struct heap_block*)((uint8_t*)ptr - sizeof(struct heap_block));
    
    if (size <= block->size) {
        return ptr; 
    }
    
    void* new_ptr = kmalloc(size);
    if (new_ptr != NULL) {
        memcpy(new_ptr, ptr, block->requested_size);
        kfree(ptr);
    }
    
    return new_ptr;
}

void heap_dump_blocks(void) {
    serial_puts("[HEAP] Heap block dump:\n");
    struct heap_block* current = kernel_heap.first_block;
    size_t block_num = 0;
    
    while (current != NULL) {
        printf("  Block %d: addr=0x%x, size=%d, used=%s, checksum=0x%x\n",
               block_num++, (uint64_t)current, current->size, 
               current->used ? "yes" : "no", current->checksum);
        current = current->next;
    }
}

void heap_print_stats(void) {
    serial_puts("[HEAP] Security Statistics:\n");
    printf("  Total allocations: %d\n", kernel_heap.stats.total_allocations);
    printf("  Total frees: %d\n", kernel_heap.stats.total_frees);
    printf("  Failed allocations: %d\n", kernel_heap.stats.failed_allocations);
    printf("  Double free attempts: %d\n", kernel_heap.stats.double_free_attempts);
    printf("  Buffer overflows detected: %d\n", kernel_heap.stats.buffer_overflow_detected);
    printf("  Use-after-free detected: %d\n", kernel_heap.stats.use_after_free_detected);
    printf("  Total corruptions: %d\n", kernel_heap.corruption_count);
}

void heap_enable_poisoning(bool enable) {
    poisoning_enabled = enable;
    serial_puts(enable ? 
        "[HEAP] Memory poisoning enabled\n" : 
        "[HEAP] Memory poisoning disabled\n");
}

void heap_enable_guard_pages(bool enable) {
    guard_pages_enabled = enable;
    serial_puts(enable ?
        "[HEAP] Guard pages enabled\n" :
        "[HEAP] Guard pages disabled\n");
}

size_t heap_get_total_size(void) { return kernel_heap.total_size; }
size_t heap_get_used_size(void) { return kernel_heap.used_size; }
size_t heap_get_free_size(void) { return kernel_heap.total_size - kernel_heap.used_size; }