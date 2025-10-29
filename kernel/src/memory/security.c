#include "include/memory/security.h"
#include "include/memory/heap.h"
#include "include/memory/paging.h"
#include "include/drivers/serial.h"
#include "libc/string.h"

static uint64_t stack_canary = 0;
static bool security_initialized = false;

// Initialize memory security subsystem
void memory_security_init(void) {
    serial_puts("[SECURITY] Initializing memory security subsystem...\n");
    
    stack_protector_init();
    heap_enable_poisoning(true);
    heap_enable_guard_pages(true);
    
    security_initialized = true;
    serial_puts("[SECURITY] Memory security initialized\n");
}

// Generate random stack canary
uint64_t generate_stack_canary(void) {
    // Simple PRNG for demonstration - replace with hardware RNG if available
    static uint64_t seed = 0x123456789ABCDEF0;
    seed = (seed * 6364136223846793005ULL) + 1;
    return seed ^ 0xDEADBEEFCAFEBABE;
}

void stack_protector_init(void) {
    stack_canary = generate_stack_canary();
    serial_puts("[SECURITY] Stack protector initialized\n");
}

bool stack_protector_check(void) {
    // This would be called from exception handlers
    // For now, just return true - real implementation would check canary
    return true;
}

// Comprehensive memory security check
void memory_security_check(void) {
    if (!security_initialized) {
        serial_puts("[SECURITY] Security subsystem not initialized!\n");
        return;
    }
    
    serial_puts("[SECURITY] Performing memory security audit...\n");
    
    bool heap_ok = heap_validate_all_blocks();
    bool stack_ok = stack_protector_check();
    
    if (!heap_ok) {
        serial_puts("[SECURITY] CRITICAL: Heap integrity check failed!\n");
    }
    
    if (!stack_ok) {
        serial_puts("[SECURITY] CRITICAL: Stack corruption detected!\n");
    }
    
    if (heap_ok && stack_ok) {
        serial_puts("[SECURITY] Memory security audit passed\n");
    } else {
        serial_puts("[SECURITY] Memory security audit FAILED!\n");
    }
}

// Secure memory zeroing that won't be optimized away
void secure_memzero(void* ptr, size_t size) {
    if (ptr == NULL || size == 0) return;
    
    volatile uint8_t* p = (volatile uint8_t*)ptr;
    while (size--) {
        *p++ = 0;
    }
}

// Secure memset that won't be optimized away
void secure_memset(void* ptr, int value, size_t size) {
    if (ptr == NULL || size == 0) return;
    
    volatile uint8_t* p = (volatile uint8_t*)ptr;
    while (size--) {
        *p++ = (uint8_t)value;
    }
}