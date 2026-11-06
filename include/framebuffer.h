/*
 * StellaOS - framebuffer.h
 * Low-level double-buffered framebuffer interface
 * Copyright (c) 2024 StellaOS Project
 */

#ifndef STELLA_FRAMEBUFFER_H
#define STELLA_FRAMEBUFFER_H

#include "types.h"

/* Initialize the framebuffer subsystem and allocate backbuffer */
void fb_init(uint32_t *addr, uint32_t width, uint32_t height, uint32_t pitch, uint8_t bpp);

/* Clear backbuffer with specified color */
void fb_clear(color_t color);

/* Put a pixel to backbuffer with alpha blending support */
void fb_putpixel(int x, int y, color_t color);

/* Read pixel from backbuffer */
color_t fb_getpixel(int x, int y);

/* Swap back buffer to front buffer */
void fb_swap(void);

/* Get dimensions and backbuffer pointer */
uint32_t fb_get_width(void);
uint32_t fb_get_height(void);
uint32_t *fb_get_backbuffer(void);

#endif /* STELLA_FRAMEBUFFER_H */
