/*
 * StellaOS - idt.h
 * Interrupt Descriptor Table (IDT) structures and gate definitions
 */

#ifndef STELLA_IDT_H
#define STELLA_IDT_H

#include "types.h"

#define IDT_NUM_ENTRIES 256

/* IDT gate entry definition */
struct idt_entry {
    uint16_t base_low;      /* Lower 16 bits of ISR address */
    uint16_t sel;           /* Kernel code segment selector (0x08) */
    uint8_t  always0;       /* Must always be zero */
    uint8_t  flags;         /* Gate flags: Present, Ring, Gate Type */
    uint16_t base_high;     /* Upper 16 bits of ISR address */
} __attribute__((packed));
typedef struct idt_entry idt_entry_t;

/* IDT pointer structure for lidt instruction */
struct idt_ptr {
    uint16_t limit;         /* Size of IDT - 1 */
    uint32_t base;          /* Base linear address of IDT table */
} __attribute__((packed));
typedef struct idt_ptr idt_ptr_t;

void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

#endif /* STELLA_IDT_H */
