/*
 * StellaOS - scheduler.h
 * Cooperative multitasking scheduler
 */

#ifndef STELLA_SCHEDULER_H
#define STELLA_SCHEDULER_H

#include "types.h"

#define MAX_TASKS 64
#define TASK_STACK_SIZE 8192
#define TASK_NAME_LEN 32

/* Task states */
typedef enum {
    TASK_STATE_READY = 0,
    TASK_STATE_RUNNING = 1,
    TASK_STATE_BLOCKED = 2,
    TASK_STATE_TERMINATED = 3
} task_state_t;

/* Task control block */
typedef struct task {
    int task_id;
    char name[TASK_NAME_LEN];
    task_state_t state;
    int priority;               /* Priority level (0-3, 0 = highest) */
    void (*entry)(void);        /* Task entry function */
    uint8_t *stack;             /* Pointer to kmalloc'd 8KB stack */
    uint32_t stack_pointer;     /* Saved ESP */
    uint32_t ticks_used;        /* Execution ticks counter */
    uint32_t sleep_until;       /* Wakeup tick count for sleeping tasks */
    bool is_used;               /* Slot allocation flag */
} task_t;

/* Scheduler API */
void scheduler_init(void);
int scheduler_create_task(const char *name, void (*entry)(void), int priority);
void scheduler_yield(void);
void scheduler_exit_task(void);
void scheduler_sleep(uint32_t ms);
int scheduler_get_task_count(void);
const char *scheduler_get_current_name(void);
int scheduler_get_current_pid(void);
task_t *scheduler_get_current_task(void);

#endif /* STELLA_SCHEDULER_H */
