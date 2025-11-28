#include "include/tasking/task.h"
#include "include/memory/heap.h"
#include "include/drivers/serial.h"
#include "libc/string.h"

static struct task *task_list = NULL;
struct task *current_task = NULL;
static uint32_t next_task_id = 1;

void task_init(struct task *t, uint64_t entry, void *stack_top, uint32_t id) {
    memset(t, 0, sizeof(struct task));
    t->rip = entry;
    t->rsp = (uint64_t)stack_top;
    t->rflags = 0x202; // IF=1 (разрешены прерывания)
    t->cs = 0x08;
    t->ss = 0x10;
    t->id = id;
    t->state = TASK_STATE_READY;
    t->stack_top = stack_top;
    t->next = NULL;
}

void task_add(struct task *t) {
    if (!task_list) {
        task_list = t;
    } else {
        struct task *last = task_list;
        while (last->next) last = last->next;
        last->next = t;
    }
}

static struct task *task_get_next_ready(void) {
    if (!current_task) return task_list;

    struct task *t = current_task->next;
    while (t) {
        if (t->state == TASK_STATE_READY) return t;
        t = t->next;
    }

    t = task_list;
    while (t && t != current_task) {
        if (t->state == TASK_STATE_READY) return t;
        t = t->next;
    }
    return NULL;
}

void tasking_init(void) {
    serial_puts("[TASK] Tasking subsystem initialized\n");
}

void task_scheduler_tick(void) {
    if (!current_task) return;

    struct task *next = task_get_next_ready();
    if (!next || next == current_task) return;

    extern void save_current_context(struct task* t);
    extern void switch_to_task(struct task *t);

    save_current_context(current_task);
    current_task->state = TASK_STATE_READY;
    next->state = TASK_STATE_RUNNING;
    switch_to_task(next);

    current_task = next;
}