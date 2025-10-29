#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define HEAP_START         0xFFFFFFFF90000000
#define HEAP_INITIAL_SIZE  0x100000
#define HEAP_MAX_SIZE      0x10000000

// Security constants
#define HEAP_CANARY_VALUE          0xDEADBEEFCAFEBABE
#define HEAP_MAX_ALLOC_SIZE        (HEAP_INITIAL_SIZE / 4)  // Max 25% of heap per allocation
#define HEAP_MIN_ALLOC_SIZE        8
#define HEAP_ALIGNMENT             8

// Memory protection flags
#define HEAP_READ_ONLY     0x01
#define HEAP_NO_EXECUTE    0x02
#define HEAP_GUARD_PAGE    0x04

// Memory poisoning
#define HEAP_ALLOC_POISON  0xAA
#define HEAP_FREE_POISON   0xDD

// Security statistics structure
struct heap_security_stats {
    size_t total_allocations;
    size_t total_frees;
    size_t failed_allocations;
    size_t double_free_attempts;
    size_t buffer_overflow_detected;
    size_t use_after_free_detected;
    size_t corruption_count;
};

// Structure with security features
struct heap_block {
    uint64_t canary_start;     // Stack canary pattern
    size_t size;
    size_t requested_size;     // Original requested size
    bool used;
    uint8_t protection_flags;
    uint32_t checksum;         // CRC-like checksum
    struct heap_block* next;
    struct heap_block* prev;
    void* allocation_ptr;      // Pointer to actual allocation
    uint64_t canary_end;       // End canary
} __attribute__((packed));

struct heap {
    void* start;
    void* end;
    size_t total_size;
    size_t used_size;
    size_t alloc_count;
    size_t free_count;
    size_t corruption_count;
    struct heap_block* first_block;
    
    // Security statistics
    struct heap_security_stats stats;
};

// Security functions
void heap_init(void);
void* kmalloc(size_t size);
void* kmalloc_protected(size_t size, uint8_t protection_flags);
void* kcalloc(size_t num, size_t size);
void* krealloc(void* ptr, size_t size);
void kfree(void* ptr);

// Security validation functions
bool heap_validate_block(struct heap_block* block);
bool heap_validate_all_blocks(void);
void heap_check_integrity(void);
bool heap_is_valid_pointer(void* ptr);

// Memory protection
bool heap_protect_block(void* ptr, uint8_t protection_flags);
bool heap_unprotect_block(void* ptr);
bool heap_mark_as_executable(void* ptr);
bool heap_mark_as_readonly(void* ptr);

// Debug and diagnostics
size_t heap_get_total_size(void);
size_t heap_get_used_size(void);
size_t heap_get_free_size(void);
void heap_dump_blocks(void);
void heap_print_stats(void);
void heap_enable_poisoning(bool enable);
void heap_enable_guard_pages(bool enable);

// Advanced security
void* kmalloc_aligned(size_t size, size_t alignment);
bool heap_detect_corruption(void);
void heap_emergency_lockdown(void);

#endif