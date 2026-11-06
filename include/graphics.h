/*
 * StellaOS - graphics.h
 * 2D graphics primitives with alpha blending and clipping
 * Copyright (c) 2024 StellaOS Project
 */

#ifndef STELLA_GRAPHICS_H
#define STELLA_GRAPHICS_H

#include "types.h"

/* Basic drawing primitives */
void gfx_fill_rect(int x, int y, int w, int h, color_t color);
void gfx_draw_rect(int x, int y, int w, int h, color_t color, int thickness);
void gfx_fill_rounded_rect(int x, int y, int w, int h, int radius, color_t color);
void gfx_draw_rounded_rect(int x, int y, int w, int h, int radius, color_t color, int thickness);
void gfx_fill_circle(int cx, int cy, int r, color_t color);
void gfx_draw_circle(int cx, int cy, int r, color_t color);
void gfx_draw_line(int x1, int y1, int x2, int y2, color_t color);

/* Gradients and effects */
void gfx_fill_gradient_v(int x, int y, int w, int h, color_t top, color_t bottom);
void gfx_fill_gradient_h(int x, int y, int w, int h, color_t left, color_t right);
void gfx_draw_shadow(int x, int y, int w, int h, int radius, int shadow_size);

/* Color operations */
color_t gfx_blend(color_t bg, color_t fg);  // alpha blend fg over bg
color_t gfx_lerp_color(color_t a, color_t b, uint8_t t);  // t: 0-255

#endif /* STELLA_GRAPHICS_H */
