/*
 * StellaOS - timer.c
 * PIT timer implementation providing 1ms system ticks and delays
 */

#include "types.h"
#include "timer.h"
#include "irq.h"
#include "io.h"

#define PIT_FREQUENCY_BASE 1193182
#define PIT_COMMAND_PORT   0x43
#define PIT_CHANNEL0_PORT  0x40

static volatile uint32_t timer_ticks = 0;

static void timer_callback(registers_t *regs) {
    UNUSED(regs);
    timer_ticks++;
}

void timer_init(uint32_t freq) {
    if (freq == 0) {
        freq = 1000;
    }

    uint32_t divisor = PIT_FREQUENCY_BASE / freq;
    if (divisor > 65535) divisor = 65535;
    if (divisor < 1) divisor = 1;

    /*
     * 0x36 = 00110110b:
     * Bits 7-6 = 00 (Channel 0)
     * Bits 5-4 = 11 (Access mode: lobyte/hibyte)
     * Bits 3-1 = 011 (Mode 3: square wave generator)
     * Bit 0    = 0 (16-bit binary)
     */
    outb(PIT_COMMAND_PORT, 0x36);

    /* Send divisor bytes */
    outb(PIT_CHANNEL0_PORT, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0_PORT, (uint8_t)((divisor >> 8) & 0xFF));

    /* Register IRQ 0 handler */
    irq_install_handler(IRQ0, timer_callback);
}

uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

void timer_wait(uint32_t ms) {
    uint32_t target = timer_ticks + ms;
    while (timer_ticks < target) {
        __asm__ volatile ("sti; hlt");
    }
}
