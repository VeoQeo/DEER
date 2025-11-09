#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define HEAP_START         0xFFFFFFFF90000000
#define HEAP_INITIAL_SIZE  0x100000
#define HEAP_MAX_SIZE      0x10000000

#define HEAP_CANARY_VALUE          0xDEADBEEFCAFEBABE
#define HEAP_MAX_ALLOC_SIZE        (HEAP_INITIAL_SIZE / 4) 
#define HEAP_MIN_ALLOC_SIZE        8
#define HEAP_ALIGNMENT             8

#define HEAP_READ_ONLY     0x01
#define HEAP_NO_EXECUTE    0x02
#define HEAP_GUARD_PAGE    0x04

#define HEAP_ALLOC_POISON  0xAA
#define HEAP_FREE_POISON   0xDD

struct heap_security_stats {
    size_t total_allocations;
    size_t total_frees;
    size_t failed_allocations;
    size_t double_free_attempts;
    size_t buffer_overflow_detected;
    size_t use_after_free_detected;
    size_t corruption_count;
};

struct heap_block {
    uint64_t canary_start;     
    size_t size;
    size_t requested_size;     
    bool used;
    uint8_t protection_flags;
    uint32_t checksum;         
    struct heap_block* next;
    struct heap_block* prev;
    void* allocation_ptr;      
    uint64_t canary_end;       
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
    
    struct heap_security_stats stats;
};

void heap_init(void);
void* kmalloc(size_t size);
void* kmalloc_protected(size_t size, uint8_t protection_flags);
void* kcalloc(size_t num, size_t size);
void* krealloc(void* ptr, size_t size);
void kfree(void* ptr);

bool heap_validate_block(struct heap_block* block);
bool heap_validate_all_blocks(void);
bool heap_is_valid_pointer(void* ptr);

size_t heap_get_total_size(void);
size_t heap_get_used_size(void);
size_t heap_get_free_size(void);
void heap_dump_blocks(void);
void heap_print_stats(void);
void heap_enable_poisoning(bool enable);
void heap_enable_guard_pages(bool enable);

void heap_emergency_lockdown(void);

#endif