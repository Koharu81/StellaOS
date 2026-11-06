/*
 * StellaOS - syscall.c
 * System call dispatcher and interrupt 0x80 handler implementation
 */

#include "types.h"
#include "syscall.h"
#include "idt.h"
#include "isr.h"
#include "scheduler.h"
#include "vfs.h"
#include "memory.h"
#include "timer.h"
#include "acpi.h"
#include "serial.h"
#include "string.h"

static syscall_fn_t syscall_table[MAX_SYSCALLS];

/* Forward declaration for assembly stub */
void syscall_dispatcher(registers_t *regs);

__attribute__((naked)) static void syscall_entry(void) {
    __asm__ volatile (
        "pushl $0\n\t"              /* Dummy error code */
        "pushl $0x80\n\t"           /* Interrupt number */
        "pushal\n\t"                /* Save EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX */
        "movw %%ds, %%ax\n\t"
        "pushl %%eax\n\t"           /* Save DS */
        "movw $0x10, %%ax\n\t"      /* Kernel data segment 0x10 */
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"
        "pushl %%esp\n\t"           /* Pass pointer to registers_t */
        "call syscall_dispatcher\n\t"
        "addl $4, %%esp\n\t"
        "popl %%eax\n\t"            /* Restore DS */
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"
        "popal\n\t"                 /* Restore registers (eax holds return value) */
        "addl $8, %%esp\n\t"        /* Clean up pushed int_no and error code */
        "iret\n\t"
        :
        :
        : "memory"
    );
}

void syscall_dispatcher(registers_t *regs) {
    uint32_t num = regs->eax;
    uint32_t arg1 = regs->ebx;
    uint32_t arg2 = regs->ecx;
    uint32_t arg3 = regs->edx;

    if (num < MAX_SYSCALLS && syscall_table[num] != NULL) {
        regs->eax = (uint32_t)syscall_table[num](arg1, arg2, arg3);
    } else {
        serial_printf("Syscall: Invalid syscall number %u called\n", num);
        regs->eax = (uint32_t)-1;
    }
}

/* Built-in system call implementations */
static int32_t sys_impl_exit(uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    UNUSED(arg1); UNUSED(arg2); UNUSED(arg3);
    scheduler_exit_task();
    return 0;
}

static int32_t sys_impl_write(uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    return vfs_write((int)arg1, (const void *)arg2, (size_t)arg3);
}

static int32_t sys_impl_read(uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    return vfs_read((int)arg1, (void *)arg2, (size_t)arg3);
}

static int32_t sys_impl_open(uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    UNUSED(arg3);
    return vfs_open((const char *)arg1, (int)arg2);
}

static int32_t sys_impl_close(uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    UNUSED(arg2); UNUSED(arg3);
    return vfs_close((int)arg1);
}

static int32_t sys_impl_malloc(uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    UNUSED(arg2); UNUSED(arg3);
    return (int32_t)(uintptr_t)kmalloc((size_t)arg1);
}

static int32_t sys_impl_free(uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    UNUSED(arg2); UNUSED(arg3);
    kfree((void *)arg1);
    return 0;
}

static int32_t sys_impl_getpid(uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    UNUSED(arg1); UNUSED(arg2); UNUSED(arg3);
    return (int32_t)scheduler_get_current_pid();
}

static int32_t sys_impl_sleep(uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    UNUSED(arg2); UNUSED(arg3);
    scheduler_sleep(arg1);
    return 0;
}

static int32_t sys_impl_time(uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    UNUSED(arg1); UNUSED(arg2); UNUSED(arg3);
    return (int32_t)timer_get_ticks();
}

static int32_t sys_impl_shutdown(uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    UNUSED(arg1); UNUSED(arg2); UNUSED(arg3);
    acpi_shutdown();
    return 0;
}

static int32_t sys_impl_reboot(uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    UNUSED(arg1); UNUSED(arg2); UNUSED(arg3);
    acpi_reboot();
    return 0;
}

void syscall_register(uint32_t num, syscall_fn_t handler) {
    if (num < MAX_SYSCALLS) {
        syscall_table[num] = handler;
    }
}

void syscall_init(void) {
    memset(syscall_table, 0, sizeof(syscall_table));

    /* Register standard system call implementations */
    syscall_register(SYS_EXIT,     sys_impl_exit);
    syscall_register(SYS_WRITE,    sys_impl_write);
    syscall_register(SYS_READ,     sys_impl_read);
    syscall_register(SYS_OPEN,     sys_impl_open);
    syscall_register(SYS_CLOSE,    sys_impl_close);
    syscall_register(SYS_MALLOC,   sys_impl_malloc);
    syscall_register(SYS_FREE,     sys_impl_free);
    syscall_register(SYS_GETPID,   sys_impl_getpid);
    syscall_register(SYS_SLEEP,    sys_impl_sleep);
    syscall_register(SYS_TIME,     sys_impl_time);
    syscall_register(SYS_SHUTDOWN, sys_impl_shutdown);
    syscall_register(SYS_REBOOT,   sys_impl_reboot);

    /* Install IDT gate for interrupt 0x80 (0xEE: present, ring 3, 32-bit interrupt gate) */
    idt_set_gate(0x80, (uint32_t)syscall_entry, 0x08, 0xEE);

    serial_printf("Syscall: Initialized system call interface on interrupt 0x80 (32 handlers)\n");
}
