/*
 * StellaOS - idt.c
 * Interrupt Descriptor Table setup and gate management
 */

#include "types.h"
#include "idt.h"
#include "isr.h"
#include "irq.h"
#include "string.h"

static idt_entry_t idt_entries[IDT_NUM_ENTRIES];
static idt_ptr_t   idt_ptr;

/* Assembly function in idt_load.asm */
extern void idt_load(uint32_t idt_ptr_addr);

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_low  = (uint16_t)(base & 0xFFFF);
    idt_entries[num].base_high = (uint16_t)((base >> 16) & 0xFFFF);

    idt_entries[num].sel       = sel;
    idt_entries[num].always0   = 0;
    idt_entries[num].flags     = flags;
}

void idt_init(void) {
    idt_ptr.limit = (uint16_t)((sizeof(idt_entry_t) * IDT_NUM_ENTRIES) - 1);
    idt_ptr.base  = (uint32_t)&idt_entries;

    /* Initialize all IDT gates to zero */
    memset(&idt_entries, 0, sizeof(idt_entry_t) * IDT_NUM_ENTRIES);

    /* Install default CPU exception handlers (0-31) */
    isr_init();

    /* Remap PIC and install IRQ handlers (32-47) */
    irq_init();

    /* Load the IDTR */
    idt_load((uint32_t)&idt_ptr);
}
