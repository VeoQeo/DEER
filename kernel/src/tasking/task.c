/**
 * @file task.c
 * @brief Minimal task (thread/process) management implementation for x86-64 bare-metal kernels.
 *
 * This file provides foundational data structures and functions for cooperative multitasking
 * in a 64-bit kernel environment. It defines a task context structure that captures the
 * CPU state (general-purpose registers, instruction pointer, stack pointer, and flags),
 * and supports basic context switching via assembly helpers.
 *
 * Tasks are assumed to run in kernel space with a flat memory model. The implementation
 * relies on a predefined Global Descriptor Table (GDT) where:
 *   - Code segment selector = 0x08
 *   - Data segment selector = 0x10
 *
 * Currently supports:
 *   - Task initialization with entry point and kernel stack
 *   - Cooperative yielding between tasks (round-robin style)
 *   - Full register state save/restore via `save_current_context()` and `switch_to_task()`
 *
 * Limitations:
 *   - No preemption (purely cooperative scheduling)
 *   - No memory protection or user-mode support
 *   - No dynamic task creation/destruction (caller manages memory)
 *   - No interrupt-safe context switching (disable interrupts during switch in production)
 *
 * Integration requirements:
 *   - Companion NASM file `task_asm.asm` must provide `save_current_context` and `switch_to_task`
 *   - Caller must allocate stack space (typically 4KB–16KB) for each task
 *   - GDT must be loaded with appropriate kernel code/data segments before task execution
 *
 * Usage example:
 *   struct task t1, t2;
 *   uint8_t stack1[4096], stack2[4096];
 *   task_init(&t1, (uint64_t)task1_entry, (uint64_t)&stack1[4096], 1);
 *   task_init(&t2, (uint64_t)task2_entry, (uint64_t)&stack2[4096], 2);
 *   t1.next = &t2; t2.next = &t1;
 *   task_set_current(&t1);
 *   // Enable interrupts, then call task_yield() from within a task
 *
 * @note This module is intended for educational or foundational use in minimal kernels.
 *       In production systems, consider implementing a preemptive scheduler with timer interrupts,
 *       per-task kernel stacks, and SMP awareness.
 *
 * @author WIENTON
 * @date 2025
 */

#include <stdint.h>
#include <stdio.h>

struct task;

struct task {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;
    uint64_t rip, cs, rflags, rsp, ss;
    struct task* next;
    uint32_t id;
    int state; // 0 = ready, 1 = running, 2 = blocked, etc.
};

// current running task 
static struct task* current_task = NULL;

// assembly context switch func 
extern void switch_to_task(struct task* next_task);
extern void save_current_context(struct task* current);

// init a task
void task_init(struct task* t, uint64_t entry, uint64_t stack_top, uint32_t id) {
    
    __builtin_memset(t, 0, sizeof(struct task));

    t->rip = entry;
    t->rsp = stack_top;
    t->rflags = 0x202;
    t->cs = 0x08;
    t->ss = 0x10;
    t->id = id;
    t->state = 0;
    t->next = NULL;
}

void task_yield(void) {
    
    if(!current_task || !current_task->next) return;

    struct task* next = current_task->next; 

    if(next->state != 0) return;
    
    current_task->state = 0;
    next->state = 1;

    save_current_context(current_task);
    switch_to_task(next);

    current_task = next;

}


// set init task

void task_set_current(struct task* t) {

    t->state = 1;
    current_task = t;

}

