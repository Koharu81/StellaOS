/*
 * StellaOS - mouse.c
 * PS/2 mouse driver with packet parsing, coordinate tracking, and button status
 */

#include "types.h"
#include "mouse.h"
#include "irq.h"
#include "io.h"

#define MOUSE_DATA_PORT    0x60
#define MOUSE_STATUS_PORT  0x64
#define MOUSE_CMD_SEND     0xD4

/* State variables */
static volatile int mouse_x = 0;
static volatile int mouse_y = 0;
static volatile int mouse_bounds_x = DEFAULT_SCREEN_WIDTH - 1;
static volatile int mouse_bounds_y = DEFAULT_SCREEN_HEIGHT - 1;

static volatile bool btn_left   = false;
static volatile bool btn_right  = false;
static volatile bool btn_middle = false;

static uint8_t mouse_cycle = 0;
static uint8_t mouse_packet[3];

static void mouse_wait_write(void) {
    uint32_t timeout = 100000;
    while (timeout--) {
        if ((inb(MOUSE_STATUS_PORT) & 0x02) == 0) {
            return;
        }
        io_wait();
    }
}

static void mouse_wait_read(void) {
    uint32_t timeout = 100000;
    while (timeout--) {
        if ((inb(MOUSE_STATUS_PORT) & 0x01) == 1) {
            return;
        }
        io_wait();
    }
}

static void mouse_write(uint8_t val) {
    mouse_wait_write();
    outb(MOUSE_STATUS_PORT, MOUSE_CMD_SEND);
    mouse_wait_write();
    outb(MOUSE_DATA_PORT, val);
}

static uint8_t mouse_read(void) {
    mouse_wait_read();
    return inb(MOUSE_DATA_PORT);
}

static void mouse_callback(registers_t *regs) {
    UNUSED(regs);

    uint8_t status = inb(MOUSE_STATUS_PORT);
    /* Ensure the data is from the auxiliary PS/2 mouse (bit 5) and output buffer full (bit 0) */
    if ((status & 0x21) != 0x21) {
        return;
    }

    uint8_t byte = inb(MOUSE_DATA_PORT);

    switch (mouse_cycle) {
        case 0:
            /* Bit 3 of packet 0 must always be 1 for valid alignment */
            if ((byte & 0x08) == 0x08) {
                mouse_packet[0] = byte;
                mouse_cycle = 1;
            }
            break;

        case 1:
            mouse_packet[1] = byte;
            mouse_cycle = 2;
            break;

        case 2:
            mouse_packet[2] = byte;
            mouse_cycle = 0;

            uint8_t flags = mouse_packet[0];

            /* Discard packets with overflow flags set (bits 6 and 7) */
            if ((flags & 0xC0) == 0) {
                int32_t dx = (int32_t)mouse_packet[1];
                int32_t dy = (int32_t)mouse_packet[2];

                /* Sign-extend 9-bit values based on bits 4 and 5 */
                if (flags & 0x10) dx |= (int32_t)0xFFFFFF00;
                if (flags & 0x20) dy |= (int32_t)0xFFFFFF00;

                /* PS/2 Y is positive upwards, screen coordinates are positive downwards */
                mouse_x += dx;
                mouse_y -= dy;

                /* Clamp to screen bounds */
                if (mouse_x < 0) mouse_x = 0;
                if (mouse_x > mouse_bounds_x) mouse_x = mouse_bounds_x;
                if (mouse_y < 0) mouse_y = 0;
                if (mouse_y > mouse_bounds_y) mouse_y = mouse_bounds_y;

                btn_left   = (flags & 0x01) ? true : false;
                btn_right  = (flags & 0x02) ? true : false;
                btn_middle = (flags & 0x04) ? true : false;
            }
            break;
    }
}

void mouse_init(void) {
    /* Set initial screen coordinates to center */
    mouse_bounds_x = DEFAULT_SCREEN_WIDTH - 1;
    mouse_bounds_y = DEFAULT_SCREEN_HEIGHT - 1;
    mouse_x = mouse_bounds_x / 2;
    mouse_y = mouse_bounds_y / 2;

    /* 1. Enable auxiliary mouse port (command 0xA8) */
    mouse_wait_write();
    outb(MOUSE_STATUS_PORT, 0xA8);

    /* 2. Read PS/2 controller configuration byte */
    mouse_wait_write();
    outb(MOUSE_STATUS_PORT, 0x20);
    uint8_t status = mouse_read();

    /* Enable IRQ 12 (bit 1) and enable mouse clock line (clear bit 5) */
    status |= (1 << 1);
    status &= ~(1 << 5);

    /* Write back updated configuration byte */
    mouse_wait_write();
    outb(MOUSE_STATUS_PORT, 0x60);
    mouse_wait_write();
    outb(MOUSE_DATA_PORT, status);

    /* 3. Send command 0xF6: Set default mouse settings */
    mouse_write(0xF6);
    mouse_read(); /* Acknowledge (0xFA) */

    /* 4. Send command 0xF4: Enable data reporting */
    mouse_write(0xF4);
    mouse_read(); /* Acknowledge (0xFA) */

    /* Register IRQ 12 handler */
    irq_install_handler(IRQ12, mouse_callback);
}

int mouse_get_x(void) {
    return mouse_x;
}

int mouse_get_y(void) {
    return mouse_y;
}

bool mouse_left_button(void) {
    return btn_left;
}

bool mouse_right_button(void) {
    return btn_right;
}

bool mouse_middle_button(void) {
    return btn_middle;
}

void mouse_set_bounds(int max_x, int max_y) {
    if (max_x > 0) mouse_bounds_x = max_x;
    if (max_y > 0) mouse_bounds_y = max_y;

    if (mouse_x > mouse_bounds_x) mouse_x = mouse_bounds_x;
    if (mouse_y > mouse_bounds_y) mouse_y = mouse_bounds_y;
}
