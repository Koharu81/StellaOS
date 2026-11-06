/*
 * StellaOS - gdt.c
 * Setup Global Descriptor Table for flat protected mode memory segmentation
 */

#include "types.h"
#include "gdt.h"

#define GDT_NUM_ENTRIES 5

static gdt_entry_t gdt_entries[GDT_NUM_ENTRIES];
static gdt_ptr_t   gdt_ptr;

/* Assembly function in gdt_flush.asm */
extern void gdt_flush(uint32_t gdt_ptr_addr);

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    if (num < 0 || num >= GDT_NUM_ENTRIES) return;

    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);

    gdt_entries[num].access      = access;
}

void gdt_init(void) {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * GDT_NUM_ENTRIES) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    /* Entry 0: Null descriptor */
    gdt_set_gate(0, 0, 0, 0, 0);

    /* Entry 1: Kernel Code Segment (Base=0, Limit=4GB, Access=0x9A, Granularity=0xCF) */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    /* Entry 2: Kernel Data Segment (Base=0, Limit=4GB, Access=0x92, Granularity=0xCF) */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    /* Entry 3: User Code Segment (Base=0, Limit=4GB, Access=0xFA, Granularity=0xCF) */
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    /* Entry 4: User Data Segment (Base=0, Limit=4GB, Access=0xF2, Granularity=0xCF) */
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    /* Flush and reload segment registers */
    gdt_flush((uint32_t)&gdt_ptr);
}
