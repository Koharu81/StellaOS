/*
 * StellaOS - isr.h
 * Interrupt Service Routines and CPU exception handling
 */

#ifndef STELLA_ISR_H
#define STELLA_ISR_H

#include "types.h"

/*
 * Pushed register state on interrupt stack frame.
 * Matches the layout created by pusha, ds push, error code and int_no push,
 * followed by processor automatic pushes (eip, cs, eflags, etc.)
 */
typedef struct registers {
    uint32_t ds;                                      /* Data segment selector */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* Pushed by pusha */
    uint32_t int_no, err_code;                        /* Interrupt number and error code */
    uint32_t eip, cs, eflags, useresp, ss;            /* Pushed automatically by CPU */
} __attribute__((packed)) registers_t;

typedef void (*isr_handler_t)(registers_t *regs);

void isr_init(void);
void isr_register_handler(uint8_t n, isr_handler_t handler);
void isr_handler(registers_t *regs);

#endif /* STELLA_ISR_H */
