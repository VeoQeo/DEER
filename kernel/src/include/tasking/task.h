#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define TASK_STATE_READY    0
#define TASK_STATE_RUNNING  1
#define TASK_STATE_BLOCKED  2
#define TASK_STATE_EXITED   3

struct task {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;
    uint64_t rip, cs, rflags, rsp, ss;
    struct task *next;
    uint32_t id;
    int state;
    void *stack_top;
};

extern struct task *current_task;

void task_init(struct task *t, uint64_t entry, void *stack_top, uint32_t id);
void task_add(struct task *t); // <-- Эта строка была добавлена!
void tasking_init(void);
void task_scheduler_tick(void);

#endif