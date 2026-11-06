/*
 * StellaOS - font.h
 * 8x16 bitmap font renderer
 * Copyright (c) 2024 StellaOS Project
 */

#ifndef STELLA_FONT_H
#define STELLA_FONT_H

#include "types.h"

void font_init(void);
void font_draw_char(int x, int y, char c, color_t fg, color_t bg); // bg=0 for transparent
void font_draw_string(int x, int y, const char *str, color_t fg, color_t bg);
void font_draw_string_scaled(int x, int y, const char *str, color_t fg, color_t bg, int scale);
int font_string_width(const char *str);
int font_string_width_scaled(const char *str, int scale);
int font_get_char_width(void);
int font_get_char_height(void);

#endif /* STELLA_FONT_H */
