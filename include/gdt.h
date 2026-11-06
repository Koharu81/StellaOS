/*
 * StellaOS - gdt.h
 * Global Descriptor Table (GDT) structures and functions
 */

#ifndef STELLA_GDT_H
#define STELLA_GDT_H

#include "types.h"

/* GDT Segment Selectors */
#define GDT_KERNEL_CODE_SEL  0x08
#define GDT_KERNEL_DATA_SEL  0x10
#define GDT_USER_CODE_SEL    0x1B
#define GDT_USER_DATA_SEL    0x23

/* GDT Entry representation */
struct gdt_entry {
    uint16_t limit_low;     /* Lower 16 bits of segment limit */
    uint16_t base_low;      /* Lower 16 bits of base address */
    uint8_t  base_middle;   /* Next 8 bits of base address */
    uint8_t  access;        /* Access flags */
    uint8_t  granularity;   /* Granularity and upper 4 bits of limit */
    uint8_t  base_high;     /* High 8 bits of base address */
} __attribute__((packed));
typedef struct gdt_entry gdt_entry_t;

/* GDT pointer passed to lgdt instruction */
struct gdt_ptr {
    uint16_t limit;         /* Size of GDT table - 1 */
    uint32_t base;          /* Base linear address of GDT table */
} __attribute__((packed));
typedef struct gdt_ptr gdt_ptr_t;

void gdt_init(void);
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

#endif /* STELLA_GDT_H */
