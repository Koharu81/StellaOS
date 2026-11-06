/*
 * StellaOS - graphics.c
 * 2D graphics primitives implementation with alpha blending
 * Copyright (c) 2024 StellaOS Project
 */

#include "graphics.h"
#include "framebuffer.h"

/* Fast integer square root using Babylonian method */
static int int_sqrt(int n) {
    if (n <= 0) return 0;
    int x = n;
    int y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }
    return x;
}

color_t gfx_blend(color_t bg, color_t fg) {
    uint8_t a = COLOR_A(fg);
    if (a == 0) return bg;
    if (a == 255) return fg;

    uint8_t inv_a = 255 - a;
    uint8_t r = (uint8_t)(((uint32_t)COLOR_R(fg) * a + (uint32_t)COLOR_R(bg) * inv_a) / 255);
    uint8_t g = (uint8_t)(((uint32_t)COLOR_G(fg) * a + (uint32_t)COLOR_G(bg) * inv_a) / 255);
    uint8_t b = (uint8_t)(((uint32_t)COLOR_B(fg) * a + (uint32_t)COLOR_B(bg) * inv_a) / 255);
    uint8_t out_a = (uint8_t)(a + ((uint32_t)COLOR_A(bg) * inv_a) / 255);

    return COLOR_ARGB(out_a, r, g, b);
}

color_t gfx_lerp_color(color_t a, color_t b, uint8_t t) {
    uint8_t inv_t = 255 - t;
    uint8_t alpha = (uint8_t)(((uint32_t)COLOR_A(a) * inv_t + (uint32_t)COLOR_A(b) * t) / 255);
    uint8_t r     = (uint8_t)(((uint32_t)COLOR_R(a) * inv_t + (uint32_t)COLOR_R(b) * t) / 255);
    uint8_t g     = (uint8_t)(((uint32_t)COLOR_G(a) * inv_t + (uint32_t)COLOR_G(b) * t) / 255);
    uint8_t bl    = (uint8_t)(((uint32_t)COLOR_B(a) * inv_t + (uint32_t)COLOR_B(b) * t) / 255);

    return COLOR_ARGB(alpha, r, g, bl);
}

void gfx_fill_rect(int x, int y, int w, int h, color_t color) {
    if (w <= 0 || h <= 0) return;

    int sw = (int)fb_get_width();
    int sh = (int)fb_get_height();

    /* Clip against screen bounds */
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x + w > sw) {
        w = sw - x;
    }
    if (y + h > sh) {
        h = sh - y;
    }
    if (w <= 0 || h <= 0) return;

    uint8_t a = COLOR_A(color);
    if (a == 0) return;

    uint32_t *bb = fb_get_backbuffer();
    if (!bb) return;

    if (a == 255) {
        /* Fast opaque fill */
        for (int r = y; r < y + h; r++) {
            uint32_t *row = bb + (uint32_t)r * (uint32_t)sw + (uint32_t)x;
            for (int c = 0; c < w; c++) {
                row[c] = color;
            }
        }
    } else {
        /* Alpha blended fill */
        for (int r = y; r < y + h; r++) {
            for (int c = x; c < x + w; c++) {
                fb_putpixel(c, r, color);
            }
        }
    }
}

void gfx_draw_rect(int x, int y, int w, int h, color_t color, int thickness) {
    if (thickness <= 0 || w <= 0 || h <= 0) return;
    if (thickness * 2 >= w || thickness * 2 >= h) {
        gfx_fill_rect(x, y, w, h, color);
        return;
    }

    gfx_fill_rect(x, y, w, thickness, color);
    gfx_fill_rect(x, y + h - thickness, w, thickness, color);
    gfx_fill_rect(x, y + thickness, thickness, h - 2 * thickness, color);
    gfx_fill_rect(x + w - thickness, y + thickness, thickness, h - 2 * thickness, color);
}

void gfx_fill_rounded_rect(int x, int y, int w, int h, int radius, color_t color) {
    if (w <= 0 || h <= 0) return;
    if (radius <= 0) {
        gfx_fill_rect(x, y, w, h, color);
        return;
    }

    if (radius * 2 > w) radius = w / 2;
    if (radius * 2 > h) radius = h / 2;

    int r2 = radius * radius;

    for (int row = 0; row < h; row++) {
        int cur_y = y + row;
        int span_x = x;
        int span_w = w;

        if (row < radius) {
            int dy = radius - 1 - row;
            int max_dx = int_sqrt(r2 - dy * dy);
            int indent = radius - max_dx;
            span_x = x + indent;
            span_w = w - 2 * indent;
        } else if (row >= h - radius) {
            int dy = row - (h - radius);
            int max_dx = int_sqrt(r2 - dy * dy);
            int indent = radius - max_dx;
            span_x = x + indent;
            span_w = w - 2 * indent;
        }

        if (span_w > 0) {
            gfx_fill_rect(span_x, cur_y, span_w, 1, color);
        }
    }
}

/* Helper to draw four corner arcs of a rounded rectangle outline */
static void draw_rounded_corners_outline(int x, int y, int w, int h, int radius, color_t color) {
    int cx_tl = x + radius;
    int cy_tl = y + radius;
    int cx_tr = x + w - radius - 1;
    int cy_tr = y + radius;
    int cx_bl = x + radius;
    int cy_bl = y + h - radius - 1;
    int cx_br = x + w - radius - 1;
    int cy_br = y + h - radius - 1;

    int ox = 0;
    int oy = radius;
    int d = 3 - 2 * radius;

    while (oy >= ox) {
        /* Top-Left Quadrant */
        fb_putpixel(cx_tl - ox, cy_tl - oy, color);
        fb_putpixel(cx_tl - oy, cy_tl - ox, color);

        /* Top-Right Quadrant */
        fb_putpixel(cx_tr + ox, cy_tr - oy, color);
        fb_putpixel(cx_tr + oy, cy_tr - ox, color);

        /* Bottom-Left Quadrant */
        fb_putpixel(cx_bl - ox, cy_bl + oy, color);
        fb_putpixel(cx_bl - oy, cy_bl + ox, color);

        /* Bottom-Right Quadrant */
        fb_putpixel(cx_br + ox, cy_br + oy, color);
        fb_putpixel(cx_br + oy, cy_br + ox, color);

        ox++;
        if (d > 0) {
            oy--;
            d = d + 4 * (ox - oy) + 10;
        } else {
            d = d + 4 * ox + 6;
        }
    }
}

void gfx_draw_rounded_rect(int x, int y, int w, int h, int radius, color_t color, int thickness) {
    if (thickness <= 0 || w <= 0 || h <= 0) return;
    if (radius <= 0) {
        gfx_draw_rect(x, y, w, h, color, thickness);
        return;
    }

    if (radius * 2 > w) radius = w / 2;
    if (radius * 2 > h) radius = h / 2;

    for (int t = 0; t < thickness; t++) {
        int cur_x = x + t;
        int cur_y = y + t;
        int cur_w = w - 2 * t;
        int cur_h = h - 2 * t;
        int cur_r = radius > t ? radius - t : 0;

        if (cur_w <= 0 || cur_h <= 0) break;

        if (cur_r <= 0) {
            gfx_draw_rect(cur_x, cur_y, cur_w, cur_h, color, 1);
            continue;
        }

        /* Straight segments */
        gfx_fill_rect(cur_x + cur_r, cur_y, cur_w - 2 * cur_r, 1, color);
        gfx_fill_rect(cur_x + cur_r, cur_y + cur_h - 1, cur_w - 2 * cur_r, 1, color);
        gfx_fill_rect(cur_x, cur_y + cur_r, 1, cur_h - 2 * cur_r, color);
        gfx_fill_rect(cur_x + cur_w - 1, cur_y + cur_r, 1, cur_h - 2 * cur_r, color);

        /* Rounded corner arcs */
        draw_rounded_corners_outline(cur_x, cur_y, cur_w, cur_h, cur_r, color);
    }
}

void gfx_fill_circle(int cx, int cy, int r, color_t color) {
    if (r < 0) return;
    if (r == 0) {
        fb_putpixel(cx, cy, color);
        return;
    }

    int r2 = r * r;
    for (int dy = -r; dy <= r; dy++) {
        int dx = int_sqrt(r2 - dy * dy);
        gfx_fill_rect(cx - dx, cy + dy, 2 * dx + 1, 1, color);
    }
}

void gfx_draw_circle(int cx, int cy, int r, color_t color) {
    if (r < 0) return;
    if (r == 0) {
        fb_putpixel(cx, cy, color);
        return;
    }

    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    while (y >= x) {
        fb_putpixel(cx + x, cy + y, color);
        fb_putpixel(cx - x, cy + y, color);
        fb_putpixel(cx + x, cy - y, color);
        fb_putpixel(cx - x, cy - y, color);
        fb_putpixel(cx + y, cy + x, color);
        fb_putpixel(cx - y, cy + x, color);
        fb_putpixel(cx + y, cy - x, color);
        fb_putpixel(cx - y, cy - x, color);

        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

void gfx_draw_line(int x1, int y1, int x2, int y2, color_t color) {
    int dx = ABS(x2 - x1);
    int dy = -ABS(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;

    while (1) {
        fb_putpixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x1 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void gfx_fill_gradient_v(int x, int y, int w, int h, color_t top, color_t bottom) {
    if (w <= 0 || h <= 0) return;

    for (int i = 0; i < h; i++) {
        uint8_t t = (uint8_t)((i * 255) / (h > 1 ? (h - 1) : 1));
        color_t c = gfx_lerp_color(top, bottom, t);
        gfx_fill_rect(x, y + i, w, 1, c);
    }
}

void gfx_fill_gradient_h(int x, int y, int w, int h, color_t left, color_t right) {
    if (w <= 0 || h <= 0) return;

    for (int j = 0; j < w; j++) {
        uint8_t t = (uint8_t)((j * 255) / (w > 1 ? (w - 1) : 1));
        color_t c = gfx_lerp_color(left, right, t);
        gfx_fill_rect(x + j, y, 1, h, c);
    }
}

void gfx_draw_shadow(int x, int y, int w, int h, int radius, int shadow_size) {
    if (shadow_size <= 0 || w <= 0 || h <= 0) return;

    /* Draw multiple semi-transparent rounded rects offset down-right with decreasing opacity */
    for (int s = shadow_size; s >= 1; s--) {
        uint8_t alpha = (uint8_t)(6 + (28 * (shadow_size - s + 1)) / (shadow_size + 1));
        int off_x = (s + 1) / 2;
        int off_y = s;
        int pad = s / 2;

        gfx_fill_rounded_rect(
            x + off_x - pad,
            y + off_y - pad,
            w + pad * 2,
            h + pad * 2,
            radius + pad,
            COLOR_ARGB(alpha, 0, 0, 0)
        );
    }
}
