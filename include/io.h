/*
 * StellaOS - io.h
 * Port I/O and hardware bus communication primitives
 */

#ifndef STELLA_IO_H
#define STELLA_IO_H

#include "types.h"

/* 8-bit port I/O */
void outb(uint16_t port, uint8_t val);
uint8_t inb(uint16_t port);

/* 16-bit port I/O */
void outw(uint16_t port, uint16_t val);
uint16_t inw(uint16_t port);

/* 32-bit port I/O */
void outl(uint16_t port, uint32_t val);
uint32_t inl(uint16_t port);

/* Wait for I/O operations to complete (approx 1-4 microsecond delay) */
void io_wait(void);

#endif /* STELLA_IO_H */
