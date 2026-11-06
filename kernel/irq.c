/*
 * StellaOS - irq.c
 * PIC8259 controller setup, IRQ routing, and dispatch
 */

#include "types.h"
#include "irq.h"
#include "idt.h"
#include "io.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1
#define PIC_EOI      0x20

/* Assembly IRQ entry points declared in irq_asm.asm */
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

static irq_handler_t irq_routines[16] = { NULL };

static void pic_remap(int offset1, int offset2) {
    /* ICW1: Start initialization sequence in cascade mode */
    outb(PIC1_COMMAND, 0x11);
    io_wait();
    outb(PIC2_COMMAND, 0x11);
    io_wait();

    /* ICW2: Vector offset */
    outb(PIC1_DATA, (uint8_t)offset1);
    io_wait();
    outb(PIC2_DATA, (uint8_t)offset2);
    io_wait();

    /* ICW3: Master PIC IRQ2 connected to Slave PIC */
    outb(PIC1_DATA, 0x04);
    io_wait();
    outb(PIC2_DATA, 0x02);
    io_wait();

    /* ICW4: 8086 mode */
    outb(PIC1_DATA, 0x01);
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();

    /* Unmask all IRQ lines */
    outb(PIC1_DATA, 0x00);
    outb(PIC2_DATA, 0x00);
}

void irq_init(void) {
    /* Remap PIC vectors: Master -> 0x20..0x27 (32..39), Slave -> 0x28..0x2F (40..47) */
    pic_remap(0x20, 0x28);

    /* Register IRQ gates in IDT (0x8E = present, ring 0, 32-bit interrupt gate) */
    idt_set_gate(32, (uint32_t)irq0,  0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1,  0x08, 0x8E);
    idt_set_gate(34, (uint32_t)irq2,  0x08, 0x8E);
    idt_set_gate(35, (uint32_t)irq3,  0x08, 0x8E);
    idt_set_gate(36, (uint32_t)irq4,  0x08, 0x8E);
    idt_set_gate(37, (uint32_t)irq5,  0x08, 0x8E);
    idt_set_gate(38, (uint32_t)irq6,  0x08, 0x8E);
    idt_set_gate(39, (uint32_t)irq7,  0x08, 0x8E);
    idt_set_gate(40, (uint32_t)irq8,  0x08, 0x8E);
    idt_set_gate(41, (uint32_t)irq9,  0x08, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);
}

void irq_install_handler(int irq, void (*handler)(registers_t *)) {
    if (irq >= 0 && irq < 16) {
        irq_routines[irq] = handler;
    }
}

void irq_uninstall_handler(int irq) {
    if (irq >= 0 && irq < 16) {
        irq_routines[irq] = NULL;
    }
}

void irq_handler(registers_t *regs) {
    if (!regs) return;

    /* Check if an IRQ handler is registered */
    int irq_idx = (int)regs->int_no - 32;
    if (irq_idx >= 0 && irq_idx < 16) {
        irq_handler_t handler = irq_routines[irq_idx];
        if (handler != NULL) {
            handler(regs);
        }
    }

    /* Send End-of-Interrupt (EOI) to PIC controllers */
    if (regs->int_no >= 40) {
        /* IRQ came from Slave PIC: send EOI to slave */
        outb(PIC2_COMMAND, PIC_EOI);
    }
    /* Always send EOI to Master PIC */
    outb(PIC1_COMMAND, PIC_EOI);
}
