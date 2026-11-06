/*
 * StellaOS - keyboard.h
 * PS/2 keyboard controller driver
 */

#ifndef STELLA_KEYBOARD_H
#define STELLA_KEYBOARD_H

#include "types.h"

void keyboard_init(void);
char keyboard_getchar(void);
int keyboard_has_key(void);
char keyboard_read(void);
uint8_t keyboard_get_scancode(void);
bool keyboard_shift_pressed(void);
bool keyboard_ctrl_pressed(void);

#endif /* STELLA_KEYBOARD_H */
