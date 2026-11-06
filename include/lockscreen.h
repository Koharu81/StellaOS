/*
 * StellaOS - lockscreen.h
 * Lock screen and login subsystem
 * Copyright (c) 2024 StellaOS Project
 */

#ifndef STELLA_LOCKSCREEN_H
#define STELLA_LOCKSCREEN_H

#include "types.h"

void lockscreen_init(void);
void lockscreen_show(void);
bool lockscreen_is_locked(void);
void lockscreen_draw(void);
void lockscreen_handle_key(char key);

#endif /* STELLA_LOCKSCREEN_H */
