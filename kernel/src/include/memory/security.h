#ifndef MEMORY_SECURITY_H
#define MEMORY_SECURITY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Memory security subsystem
void memory_security_init(void);
void memory_security_check(void);
bool memory_validate_kernel_stack(void);
bool memory_validate_heap_integrity(void);
void memory_dump_security_status(void);

// Stack protection
void stack_protector_init(void);
bool stack_protector_check(void);
uint64_t generate_stack_canary(void);

// Address Space Layout Randomization
void aslr_init(void);
uint64_t aslr_randomize_address(uint64_t base, uint64_t size);

// Memory sanitization
void secure_memset(void* ptr, int value, size_t size);
void secure_memzero(void* ptr, size_t size);
void memory_poison_range(void* start, void* end, uint8_t pattern);

#endif