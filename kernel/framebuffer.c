/*
 * StellaOS - framebuffer.c
 * Double-buffered framebuffer implementation with alpha blending
 * Copyright (c) 2024 StellaOS Project
 */

#include "framebuffer.h"
#include "memory.h"
#include "string.h"
#include "mouse.h"

static uint32_t *g_fb_front = NULL;
static uint32_t *g_fb_back  = NULL;
static uint32_t  g_fb_width  = 0;
static uint32_t  g_fb_height = 0;
static uint32_t  g_fb_pitch  = 0;
static uint8_t   g_fb_bpp    = 32;

void fb_init(uint32_t *addr, uint32_t width, uint32_t height, uint32_t pitch, uint8_t bpp) {
    g_fb_front  = addr;
    g_fb_width  = width;
    g_fb_height = height;
    g_fb_pitch  = pitch;
    g_fb_bpp    = bpp;

    if (g_fb_back) {
        kfree(g_fb_back);
        g_fb_back = NULL;
    }

    if (width > 0 && height > 0) {
        size_t size = (size_t)width * height * sizeof(uint32_t);
        g_fb_back = (uint32_t *)kmalloc(size);
        if (g_fb_back) {
            memset(g_fb_back, 0, size);
        }
        mouse_set_bounds((int)width - 1, (int)height - 1);
    }
}

void fb_clear(color_t color) {
    if (!g_fb_back) return;
    uint32_t total = g_fb_width * g_fb_height;
    for (uint32_t i = 0; i < total; i++) {
        g_fb_back[i] = color;
    }
}

void fb_putpixel(int x, int y, color_t color) {
    if (!g_fb_back) return;
    if (x < 0 || x >= (int)g_fb_width || y < 0 || y >= (int)g_fb_height) {
        return;
    }

    uint8_t a = COLOR_A(color);
    if (a == 0) {
        return;
    }

    uint32_t idx = (uint32_t)y * g_fb_width + (uint32_t)x;

    if (a == 255) {
        g_fb_back[idx] = color;
        return;
    }

    color_t bg = g_fb_back[idx];
    uint8_t inv_a = 255 - a;

    uint8_t r = (uint8_t)(((uint32_t)COLOR_R(color) * a + (uint32_t)COLOR_R(bg) * inv_a) / 255);
    uint8_t g = (uint8_t)(((uint32_t)COLOR_G(color) * a + (uint32_t)COLOR_G(bg) * inv_a) / 255);
    uint8_t b = (uint8_t)(((uint32_t)COLOR_B(color) * a + (uint32_t)COLOR_B(bg) * inv_a) / 255);

    g_fb_back[idx] = COLOR_RGB(r, g, b);
}

color_t fb_getpixel(int x, int y) {
    if (!g_fb_back) return 0;
    if (x < 0 || x >= (int)g_fb_width || y < 0 || y >= (int)g_fb_height) {
        return 0;
    }
    return g_fb_back[(uint32_t)y * g_fb_width + (uint32_t)x];
}

void fb_swap(void) {
    if (!g_fb_front || !g_fb_back || g_fb_width == 0 || g_fb_height == 0) {
        return;
    }

    if (g_fb_pitch == g_fb_width * sizeof(uint32_t)) {
        memcpy(g_fb_front, g_fb_back, (size_t)g_fb_width * g_fb_height * sizeof(uint32_t));
    } else {
        uint8_t *dst = (uint8_t *)g_fb_front;
        uint8_t *src = (uint8_t *)g_fb_back;
        size_t row_bytes = (size_t)g_fb_width * sizeof(uint32_t);
        for (uint32_t y = 0; y < g_fb_height; y++) {
            memcpy(dst + y * g_fb_pitch, src + y * row_bytes, row_bytes);
        }
    }
}

uint32_t fb_get_width(void) {
    return g_fb_width;
}

uint32_t fb_get_height(void) {
    return g_fb_height;
}

uint32_t *fb_get_backbuffer(void) {
    return g_fb_back;
}
