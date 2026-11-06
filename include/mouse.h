/*
 * StellaOS - mouse.h
 * PS/2 mouse driver with packet parsing and boundary clamping
 */

#ifndef STELLA_MOUSE_H
#define STELLA_MOUSE_H

#include "types.h"

void mouse_init(void);
int mouse_get_x(void);
int mouse_get_y(void);
bool mouse_left_button(void);
bool mouse_right_button(void);
bool mouse_middle_button(void);
void mouse_set_bounds(int max_x, int max_y);

#endif /* STELLA_MOUSE_H */
