/*
 * StellaOS - serial.h
 * Serial COM1 debugging port driver and formatted serial printf
 */

#ifndef STELLA_SERIAL_H
#define STELLA_SERIAL_H

#include "types.h"

void serial_init(void);
void serial_putchar(char c);
void serial_write(const char *str);
void serial_printf(const char *fmt, ...);

#endif /* STELLA_SERIAL_H */
