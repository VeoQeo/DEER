#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define HEAP_START         0xFFFFFFFF90000000
#define HEAP_INITIAL_SIZE  0x100000
#define HEAP_MAX_SIZE      0x4000000
#define HEAP_ALIGNMENT     8

typedef struct heap_block {
    size_t size;
    bool used;
    struct heap_block* next;
    struct heap_block* prev;
} heap_block_t;

typedef struct {
    void* start;
    void* end;
    size_t total_size;
    size_t used_size;
    size_t block_count;
    heap_block_t* first_block;
} heap_t;

void heap_init(void);
void* kmalloc(size_t size);
void* kcalloc(size_t num, size_t size);
void* krealloc(void* ptr, size_t size);
void kfree(void* ptr);

size_t heap_get_total_size(void);
size_t heap_get_used_size(void);
size_t heap_get_free_size(void);
void heap_dump_blocks(void);

#endif