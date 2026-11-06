/*
 * StellaOS - irq.h
 * Hardware Interrupt Request (IRQ) definitions and management
 */

#ifndef STELLA_IRQ_H
#define STELLA_IRQ_H

#include "types.h"
#include "isr.h"

/* Standard Hardware IRQ Lines */
#define IRQ0  0   /* Programmable Interval Timer (PIT) */
#define IRQ1  1   /* Keyboard */
#define IRQ2  2   /* Cascade for IRQ 8-15 */
#define IRQ3  3   /* COM2 (if enabled) */
#define IRQ4  4   /* COM1 */
#define IRQ5  5   /* LPT2 / Sound Card */
#define IRQ6  6   /* Floppy Disk */
#define IRQ7  7   /* LPT1 / Spurious Interrupt */
#define IRQ8  8   /* Real Time Clock (RTC) */
#define IRQ9  9   /* ACPI / Open Interrupt */
#define IRQ10 10  /* Open Interrupt / Network */
#define IRQ11 11  /* Open Interrupt / Sound */
#define IRQ12 12  /* PS/2 Mouse */
#define IRQ13 13  /* FPU Coprocessor / Inter-processor */
#define IRQ14 14  /* Primary ATA Hard Disk */
#define IRQ15 15  /* Secondary ATA Hard Disk */

typedef void (*irq_handler_t)(registers_t *regs);

void irq_init(void);
void irq_install_handler(int irq, void (*handler)(registers_t *));
void irq_uninstall_handler(int irq);
void irq_handler(registers_t *regs);

#endif /* STELLA_IRQ_H */
