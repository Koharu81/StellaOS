/*
 * StellaOS - crash.h
 * Kawaii panic / error crash screen
 * Copyright (c) 2024 StellaOS Project
 */

#ifndef STELLA_CRASH_H
#define STELLA_CRASH_H

#include "types.h"
#include "isr.h"

void crash_screen(const char *error, const char *details, uint32_t error_code);
void crash_screen_regs(const char *error, registers_t *regs);

#endif /* STELLA_CRASH_H */
