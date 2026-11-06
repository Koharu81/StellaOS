/*
 * StellaOS - keyboard.c
 * PS/2 Keyboard driver with US layout scancode translation and key buffer
 */

#include "types.h"
#include "keyboard.h"
#include "irq.h"
#include "io.h"

#define KBD_DATA_PORT    0x60
#define KBD_STATUS_PORT  0x64
#define KBD_BUFFER_SIZE  256

/* Circular buffer for incoming ASCII keystrokes */
static char kbd_buffer[KBD_BUFFER_SIZE];
static volatile uint32_t kbd_head = 0;
static volatile uint32_t kbd_tail = 0;

/* Modifier states */
static volatile bool shift_pressed = false;
static volatile bool ctrl_pressed  = false;
static volatile bool alt_pressed   = false;
static volatile bool caps_lock     = false;
static volatile uint8_t last_scancode = 0;

/* Standard US keyboard scancode set 1 unshifted lookup table */
static const char kbd_us_normal[128] = {
    0,   27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   /* 0x1D: Left Control */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,   /* 0x2A: Left Shift */
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0,   /* 0x36: Right Shift */
    '*',
    0,   /* 0x38: Left Alt */
    ' ', /* 0x39: Space */
    0,   /* 0x3A: Caps Lock */
    /* F1 - F10: 0x3B - 0x44 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,   /* 0x45: Num Lock */
    0,   /* 0x46: Scroll Lock */
    /* Keypad 7, 8, 9, '-' */
    '7', '8', '9', '-',
    /* Keypad 4, 5, 6, '+' */
    '4', '5', '6', '+',
    /* Keypad 1, 2, 3, '0', '.' */
    '1', '2', '3', '0', '.',
    0, 0, 0,
    /* 0x57 - 0x58: F11, F12 */
    0, 0
};

/* Standard US keyboard scancode set 1 shifted lookup table */
static const char kbd_us_shifted[128] = {
    0,   27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,   /* Left Control */
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,   /* Left Shift */
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    0,   /* Right Shift */
    '*',
    0,   /* Left Alt */
    ' ', /* Space */
    0,   /* Caps Lock */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0,
    '7', '8', '9', '-',
    '4', '5', '6', '+',
    '1', '2', '3', '0', '.',
    0, 0, 0,
    0, 0
};

static void keyboard_callback(registers_t *regs) {
    UNUSED(regs);

    uint8_t scancode = inb(KBD_DATA_PORT);
    last_scancode = scancode;

    /* Check for key release (break code: high bit set) */
    if (scancode & 0x80) {
        uint8_t released = scancode & 0x7F;
        if (released == KEY_LSHIFT || released == KEY_RSHIFT) {
            shift_pressed = false;
        } else if (released == KEY_LCTRL) {
            ctrl_pressed = false;
        } else if (released == KEY_LALT) {
            alt_pressed = false;
        }
        return;
    }

    /* Key press (make code) */
    if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT) {
        shift_pressed = true;
        return;
    }
    if (scancode == KEY_LCTRL) {
        ctrl_pressed = true;
        return;
    }
    if (scancode == KEY_LALT) {
        alt_pressed = true;
        return;
    }
    if (scancode == KEY_CAPS) {
        caps_lock = !caps_lock;
        return;
    }

    /* Translate scancode to ASCII */
    if (scancode < 128) {
        char ch = 0;
        char norm = kbd_us_normal[scancode];

        if (norm >= 'a' && norm <= 'z') {
            /* For letters, Shift XOR CapsLock yields uppercase */
            bool uppercase = (shift_pressed != caps_lock);
            ch = uppercase ? kbd_us_shifted[scancode] : norm;
        } else {
            /* Non-letter symbols use shift */
            ch = shift_pressed ? kbd_us_shifted[scancode] : norm;
        }

        if (ch != 0) {
            /* Push to circular buffer if space permits */
            uint32_t next = (kbd_head + 1) % KBD_BUFFER_SIZE;
            if (next != kbd_tail) {
                kbd_buffer[kbd_head] = ch;
                kbd_head = next;
            }
        }
    }
}

void keyboard_init(void) {
    /* Flush existing PS/2 controller buffer */
    while (inb(KBD_STATUS_PORT) & 0x01) {
        inb(KBD_DATA_PORT);
    }

    /* Register IRQ 1 handler */
    irq_install_handler(IRQ1, keyboard_callback);
}

int keyboard_has_key(void) {
    return kbd_head != kbd_tail;
}

char keyboard_read(void) {
    if (kbd_head == kbd_tail) {
        return 0;
    }
    char ch = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUFFER_SIZE;
    return ch;
}

char keyboard_getchar(void) {
    while (!keyboard_has_key()) {
        __asm__ volatile ("sti; hlt");
    }
    return keyboard_read();
}

uint8_t keyboard_get_scancode(void) {
    return last_scancode;
}

bool keyboard_shift_pressed(void) {
    return shift_pressed;
}

bool keyboard_ctrl_pressed(void) {
    return ctrl_pressed;
}
