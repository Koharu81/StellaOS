/*
 * StellaOS - scheduler.c
 * Cooperative multitasking scheduler implementation
 */

#include "types.h"
#include "scheduler.h"
#include "memory.h"
#include "string.h"
#include "timer.h"
#include "serial.h"

static task_t tasks[MAX_TASKS];
static int current_task_id = 0;
static int total_tasks_count = 0;

/* Assembly context switch routine */
__attribute__((naked)) static void scheduler_switch_context(uint32_t *prev_sp __attribute__((unused)), uint32_t next_sp __attribute__((unused))) {
    __asm__ volatile (
        "movl 4(%%esp), %%eax\n\t"   /* prev_sp pointer */
        "movl 8(%%esp), %%edx\n\t"   /* next_sp value */
        "pushfl\n\t"                 /* Save EFLAGS */
        "pushal\n\t"                 /* Save EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX */
        "movl %%esp, (%%eax)\n\t"    /* *prev_sp = current esp */
        "movl %%edx, %%esp\n\t"      /* esp = next_sp */
        "popal\n\t"                  /* Restore registers */
        "popfl\n\t"                  /* Restore EFLAGS */
        "ret\n\t"                    /* Return to new task instruction pointer */
        :
        :
        : "memory"
    );
}

/* Trampoline wrapper called when a new task begins execution */
static void task_trampoline(void) {
    task_t *curr = scheduler_get_current_task();
    if (curr && curr->entry) {
        curr->entry();
    }
    scheduler_exit_task();
}

void scheduler_init(void) {
    memset(tasks, 0, sizeof(tasks));

    /* Task 0 is always the kernel / idle task */
    tasks[0].task_id = 0;
    strncpy(tasks[0].name, "kernel", TASK_NAME_LEN - 1);
    tasks[0].state = TASK_STATE_RUNNING;
    tasks[0].priority = 0;
    tasks[0].entry = NULL;
    tasks[0].stack = NULL;          /* Uses the existing boot/kernel stack */
    tasks[0].stack_pointer = 0;
    tasks[0].ticks_used = 0;
    tasks[0].sleep_until = 0;
    tasks[0].is_used = true;

    current_task_id = 0;
    total_tasks_count = 1;

    serial_printf("Scheduler: Initialized cooperative scheduler (max %d tasks)\n", MAX_TASKS);
}

int scheduler_create_task(const char *name, void (*entry)(void), int priority) {
    if (!name || !entry) {
        return -1;
    }

    /* Clamp priority between 0 and 3 */
    if (priority < 0) priority = 0;
    if (priority > 3) priority = 3;

    /* Find a free task slot (slot 0 is reserved for kernel) */
    int slot = -1;
    for (int i = 1; i < MAX_TASKS; i++) {
        if (!tasks[i].is_used) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        serial_printf("Scheduler: No free task slots available\n");
        return -1;
    }

    /* Allocate 8KB stack for this task */
    uint8_t *stack = (uint8_t *)kmalloc(TASK_STACK_SIZE);
    if (!stack) {
        serial_printf("Scheduler: Failed to allocate stack for task '%s'\n", name);
        return -1;
    }

    memset(stack, 0, TASK_STACK_SIZE);

    /* Initialize stack frame for scheduler_switch_context:
     * Stack grows downwards. Top of stack is at stack + TASK_STACK_SIZE.
     * We push:
     * 1. Target return address for 'ret' (task_trampoline)
     * 2. Initial EFLAGS (0x0202: IF enabled, bit 1 set)
     * 3. 8 registers for popal (EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)
     */
    uint32_t *sp = (uint32_t *)(stack + TASK_STACK_SIZE);

    *(--sp) = (uint32_t)task_trampoline; /* Return address for ret */
    *(--sp) = 0x00000202;                /* EFLAGS (IF = 1) */
    *(--sp) = 0x00000000;                /* EAX */
    *(--sp) = 0x00000000;                /* ECX */
    *(--sp) = 0x00000000;                /* EDX */
    *(--sp) = 0x00000000;                /* EBX */
    *(--sp) = 0x00000000;                /* ESP placeholder */
    *(--sp) = 0x00000000;                /* EBP */
    *(--sp) = 0x00000000;                /* ESI */
    *(--sp) = 0x00000000;                /* EDI */

    tasks[slot].task_id = slot;
    strncpy(tasks[slot].name, name, TASK_NAME_LEN - 1);
    tasks[slot].name[TASK_NAME_LEN - 1] = '\0';
    tasks[slot].state = TASK_STATE_READY;
    tasks[slot].priority = priority;
    tasks[slot].entry = entry;
    tasks[slot].stack = stack;
    tasks[slot].stack_pointer = (uint32_t)sp;
    tasks[slot].ticks_used = 0;
    tasks[slot].sleep_until = 0;
    tasks[slot].is_used = true;

    total_tasks_count++;

    serial_printf("Scheduler: Created task %d '%s' (prio %d, sp 0x%x)\n",
                  slot, tasks[slot].name, priority, tasks[slot].stack_pointer);

    return slot;
}

void scheduler_yield(void) {
    uint32_t current_ticks = timer_get_ticks();

    /* Check sleeping tasks and wake them up if timer expired */
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].is_used && tasks[i].state == TASK_STATE_BLOCKED) {
            if (tasks[i].sleep_until > 0 && current_ticks >= tasks[i].sleep_until) {
                tasks[i].state = TASK_STATE_READY;
                tasks[i].sleep_until = 0;
            }
        }
    }

    /* Update current task execution ticks */
    task_t *curr = &tasks[current_task_id];
    curr->ticks_used++;

    /* Find next task to run.
     * Check priorities from 0 (highest) to 3 (lowest).
     * Round-robin within the same priority level.
     */
    int next_id = -1;
    for (int prio = 0; prio <= 3; prio++) {
        for (int i = 1; i <= MAX_TASKS; i++) {
            int candidate = (current_task_id + i) % MAX_TASKS;
            if (tasks[candidate].is_used &&
                tasks[candidate].priority == prio &&
                tasks[candidate].state == TASK_STATE_READY) {
                next_id = candidate;
                break;
            }
        }
        if (next_id != -1) {
            break;
        }
    }

    /* If no ready task found, fall back to current task if still runnable */
    if (next_id == -1) {
        if (curr->state == TASK_STATE_RUNNING || curr->state == TASK_STATE_READY) {
            return; /* Continue running current task */
        }
        /* Otherwise, fall back to kernel idle task 0 */
        next_id = 0;
    }

    /* If the chosen task is already running, nothing to do */
    if (next_id == current_task_id) {
        curr->state = TASK_STATE_RUNNING;
        return;
    }

    task_t *next = &tasks[next_id];

    /* Update states */
    if (curr->state == TASK_STATE_RUNNING) {
        curr->state = TASK_STATE_READY;
    }
    next->state = TASK_STATE_RUNNING;

    int prev_id = current_task_id;
    current_task_id = next_id;

    /* Perform low-level context switch */
    scheduler_switch_context(&tasks[prev_id].stack_pointer, next->stack_pointer);
}

void scheduler_exit_task(void) {
    if (current_task_id == 0) {
        /* Kernel idle task cannot exit */
        serial_printf("Scheduler: Attempted to exit kernel idle task, ignoring\n");
        return;
    }

    task_t *curr = &tasks[current_task_id];
    serial_printf("Scheduler: Task %d '%s' terminated\n", curr->task_id, curr->name);

    curr->state = TASK_STATE_TERMINATED;
    curr->is_used = false;

    if (curr->stack) {
        kfree(curr->stack);
        curr->stack = NULL;
    }

    total_tasks_count--;

    /* Yield immediately to switch to another ready task */
    scheduler_yield();
}

void scheduler_sleep(uint32_t ms) {
    task_t *curr = &tasks[current_task_id];
    uint32_t ticks_to_sleep = ms; /* timer is 1000Hz -> 1 tick = 1 ms */
    if (ticks_to_sleep == 0) {
        ticks_to_sleep = 1;
    }

    curr->sleep_until = timer_get_ticks() + ticks_to_sleep;
    curr->state = TASK_STATE_BLOCKED;

    scheduler_yield();
}

int scheduler_get_task_count(void) {
    return total_tasks_count;
}

const char *scheduler_get_current_name(void) {
    if (current_task_id >= 0 && current_task_id < MAX_TASKS && tasks[current_task_id].is_used) {
        return tasks[current_task_id].name;
    }
    return "unknown";
}

int scheduler_get_current_pid(void) {
    return current_task_id;
}

task_t *scheduler_get_current_task(void) {
    if (current_task_id >= 0 && current_task_id < MAX_TASKS && tasks[current_task_id].is_used) {
        return &tasks[current_task_id];
    }
    return NULL;
}
