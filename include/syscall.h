/*
 * StellaOS - syscall.h
 * System call interface and dispatch table
 */

#ifndef STELLA_SYSCALL_H
#define STELLA_SYSCALL_H

#include "types.h"
#include "isr.h"

#define MAX_SYSCALLS 32

/* Standard System Call Numbers */
#define SYS_EXIT      0
#define SYS_WRITE     1
#define SYS_READ      2
#define SYS_OPEN      3
#define SYS_CLOSE     4
#define SYS_MALLOC    5
#define SYS_FREE      6
#define SYS_GETPID    7
#define SYS_SLEEP     8
#define SYS_TIME      9
#define SYS_SHUTDOWN  10
#define SYS_REBOOT    11

typedef int32_t (*syscall_fn_t)(uint32_t arg1, uint32_t arg2, uint32_t arg3);

/* Syscall API */
void syscall_init(void);
void syscall_register(uint32_t num, syscall_fn_t handler);

/* Userspace / kernel invocation helper */
static inline int32_t sys_call(uint32_t num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    int32_t ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "0"(num), "b"(arg1), "c"(arg2), "d"(arg3)
        : "memory"
    );
    return ret;
}

#endif /* STELLA_SYSCALL_H */
