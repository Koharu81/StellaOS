/*
 * StellaOS - timer.h
 * Programmable Interval Timer (PIT 8253/8254) driver
 */

#ifndef STELLA_TIMER_H
#define STELLA_TIMER_H

#include "types.h"

void timer_init(uint32_t freq);
uint32_t timer_get_ticks(void);
void timer_wait(uint32_t ms);

#endif /* STELLA_TIMER_H */
