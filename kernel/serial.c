/*
 * StellaOS - serial.c
 * COM1 serial port driver and formatted serial printf
 */

#include "types.h"
#include "serial.h"
#include "io.h"
#include "string.h"

#define COM1_PORT 0x3F8

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type)   __builtin_va_arg(ap, type)
#define va_end(ap)         __builtin_va_end(ap)

static int serial_is_transmit_empty(void) {
    return inb(COM1_PORT + 5) & 0x20;
}

void serial_init(void) {
    outb(COM1_PORT + 1, 0x00);    /* Disable all COM1 interrupts */
    outb(COM1_PORT + 3, 0x80);    /* Enable DLAB (set baud rate divisor) */
    outb(COM1_PORT + 0, 0x03);    /* Set divisor to 3 (lo byte) -> 38400 baud */
    outb(COM1_PORT + 1, 0x00);    /*                  (hi byte) */
    outb(COM1_PORT + 3, 0x03);    /* 8 bits, no parity, one stop bit */
    outb(COM1_PORT + 2, 0xC7);    /* Enable FIFO, clear them, 14-byte threshold */
    outb(COM1_PORT + 4, 0x0B);    /* IRQs enabled, RTS/DSR set */
}

void serial_putchar(char c) {
    if (c == '\n') {
        uint32_t timeout = 100000;
        while (!serial_is_transmit_empty() && --timeout) {
            io_wait();
        }
        outb(COM1_PORT, '\r');
    }

    uint32_t timeout = 100000;
    while (!serial_is_transmit_empty() && --timeout) {
        io_wait();
    }
    outb(COM1_PORT, (uint8_t)c);
}

void serial_write(const char *str) {
    if (!str) return;
    while (*str != '\0') {
        serial_putchar(*str++);
    }
}

void serial_printf(const char *fmt, ...) {
    if (!fmt) return;

    va_list args;
    va_start(args, fmt);

    char num_buf[36];

    for (const char *p = fmt; *p != '\0'; p++) {
        if (*p != '%') {
            serial_putchar(*p);
            continue;
        }

        p++; /* Skip '%' */
        if (*p == '\0') {
            break;
        }

        switch (*p) {
            case 'c': {
                char ch = (char)va_arg(args, int);
                serial_putchar(ch);
                break;
            }

            case 's': {
                const char *s = va_arg(args, const char *);
                if (s == NULL) {
                    serial_write("(null)");
                } else {
                    serial_write(s);
                }
                break;
            }

            case 'd':
            case 'i': {
                int val = va_arg(args, int);
                itoa(val, num_buf, 10);
                serial_write(num_buf);
                break;
            }

            case 'u': {
                uint32_t val = va_arg(args, uint32_t);
                utoa(val, num_buf, 10);
                serial_write(num_buf);
                break;
            }

            case 'x': {
                uint32_t val = va_arg(args, uint32_t);
                utoa(val, num_buf, 16);
                serial_write(num_buf);
                break;
            }

            case 'X': {
                uint32_t val = va_arg(args, uint32_t);
                utoa(val, num_buf, 16);
                int idx;
                for (idx = 0; num_buf[idx] != '\0'; idx++) {
                    if (num_buf[idx] >= 'a' && num_buf[idx] <= 'z') {
                        num_buf[idx] -= 32;
                    }
                }
                serial_write(num_buf);
                break;
            }

            case 'p': {
                uint32_t ptr_val = (uint32_t)va_arg(args, void *);
                serial_write("0x");
                utoa(ptr_val, num_buf, 16);
                serial_write(num_buf);
                break;
            }

            case '%': {
                serial_putchar('%');
                break;
            }

            default: {
                serial_putchar('%');
                serial_putchar(*p);
                break;
            }
        }
    }

    va_end(args);
}
