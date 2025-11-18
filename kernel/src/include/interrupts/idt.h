#ifndef IDT_H
#define IDT_H

#include <stdint.h>

struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t base_middle;
    uint32_t base_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));
extern struct idt_ptr idtp;

#define IDT_PRESENT     (1 << 7)
#define IDT_RING0       (0 << 5)
#define IDT_RING3       (3 << 5)
#define IDT_TYPE_INT    0x0E    
#define IDT_TYPE_TRAP   0x0F    

// Исключения x86_64
#define EXCEPTION_DIVISION_ERROR        0
#define EXCEPTION_DEBUG                 1
#define EXCEPTION_NMI                   2
#define EXCEPTION_BREAKPOINT            3
#define EXCEPTION_OVERFLOW              4
#define EXCEPTION_BOUND_RANGE           5
#define EXCEPTION_INVALID_OPCODE        6
#define EXCEPTION_DEVICE_NOT_AVAILABLE  7
#define EXCEPTION_DOUBLE_FAULT          8
#define EXCEPTION_COPROCESSOR_SEGMENT   9
#define EXCEPTION_INVALID_TSS           10
#define EXCEPTION_SEGMENT_NOT_PRESENT   11
#define EXCEPTION_STACK_SEGMENT_FAULT   12
#define EXCEPTION_GPF                   13
#define EXCEPTION_PAGE_FAULT            14
#define EXCEPTION_RESERVED              15
#define EXCEPTION_X87_FPU               16
#define EXCEPTION_ALIGNMENT_CHECK       17
#define EXCEPTION_MACHINE_CHECK         18
#define EXCEPTION_SIMD_FPU              19
#define EXCEPTION_VIRTUALIZATION        20
#define EXCEPTION_SECURITY              30

void idt_init(void);
void idt_load(void);
void idt_set_entry(uint8_t index, uint64_t base, uint16_t selector, uint8_t type_attr);

#endif // IDT_H