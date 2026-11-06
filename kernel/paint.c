/*
 * StellaOS - paint.c
 * Kawaii desktop paint and drawing application
 * Copyright (c) 2024 StellaOS Project
 */

#include "paint.h"
#include "gui.h"
#include "graphics.h"
#include "font.h"
#include "framebuffer.h"
#include "memory.h"
#include "string.h"
#include "mouse.h"

#define CANVAS_W 496
#define CANVAS_H 328
#define PALETTE_COUNT 12
#define BRUSH_SIZES_COUNT 4

typedef struct {
    color_t *canvas;
    color_t current_color;
    int current_size;    /* 1, 3, 5, 8 */
    int prev_draw_x;
    int prev_draw_y;
    bool is_drawing;
} paint_state_t;

static window_t *g_paint_win = NULL;
static paint_state_t g_paint;

static const color_t g_palette[PALETTE_COUNT] = {
    COLOR_RGB(255, 50, 50),    /* Red */
    COLOR_RGB(46, 204, 113),   /* Green */
    COLOR_RGB(52, 152, 219),   /* Blue */
    COLOR_RGB(241, 196, 15),   /* Yellow */
    COLOR_RGB(26, 188, 156),   /* Cyan */
    COLOR_RGB(155, 89, 182),   /* Magenta */
    COLOR_RGB(255, 255, 255),  /* White */
    COLOR_RGB(20, 20, 25),     /* Black */
    COLOR_RGB(255, 105, 180),  /* Pink */
    COLOR_RGB(230, 126, 34),   /* Orange */
    COLOR_RGB(180, 140, 230),  /* Purple */
    COLOR_RGB(139, 69, 19)     /* Brown */
};

static const int g_brush_sizes[BRUSH_SIZES_COUNT] = { 1, 3, 5, 8 };

static void paint_stamp_point(int cx, int cy) {
    if (!g_paint.canvas) return;

    int r = g_paint.current_size / 2;
    if (r < 1) {
        if (cx >= 0 && cx < CANVAS_W && cy >= 0 && cy < CANVAS_H) {
            g_paint.canvas[cy * CANVAS_W + cx] = g_paint.current_color;
        }
        return;
    }

    int r2 = r * r;
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (dx * dx + dy * dy <= r2) {
                int px = cx + dx;
                int py = cy + dy;
                if (px >= 0 && px < CANVAS_W && py >= 0 && py < CANVAS_H) {
                    g_paint.canvas[py * CANVAS_W + px] = g_paint.current_color;
                }
            }
        }
    }
}

static void paint_draw_line(int x0, int y0, int x1, int y1) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int abs_dx = dx < 0 ? -dx : dx;
    int abs_dy = dy < 0 ? -dy : dy;
    int steps = abs_dx > abs_dy ? abs_dx : abs_dy;

    if (steps == 0) {
        paint_stamp_point(x0, y0);
        return;
    }

    for (int i = 0; i <= steps; i++) {
        int cx = x0 + (dx * i) / steps;
        int cy = y0 + (dy * i) / steps;
        paint_stamp_point(cx, cy);
    }
}

static void paint_clear_canvas(void) {
    if (!g_paint.canvas) return;
    for (int i = 0; i < CANVAS_W * CANVAS_H; i++) {
        g_paint.canvas[i] = COLOR_RGB(255, 255, 255);
    }
}

static void paint_draw(window_t *win) {
    int start_x = win->x;
    int start_y = win->y + WINDOW_TITLE_HEIGHT;
    int win_w   = win->width;

    int mx = mouse_get_x();
    int my = mouse_get_y();
    int lb = mouse_left_button();

    int canvas_x = start_x + 12;
    int canvas_y = start_y + 48;

    /* Handle real-time dragging inside canvas */
    if (lb && g_paint.canvas) {
        int rel_cx = mx - canvas_x;
        int rel_cy = my - canvas_y;

        if (rel_cx >= 0 && rel_cx < CANVAS_W && rel_cy >= 0 && rel_cy < CANVAS_H) {
            if (g_paint.is_drawing && g_paint.prev_draw_x >= 0) {
                paint_draw_line(g_paint.prev_draw_x, g_paint.prev_draw_y, rel_cx, rel_cy);
            } else {
                paint_stamp_point(rel_cx, rel_cy);
                g_paint.is_drawing = true;
            }
            g_paint.prev_draw_x = rel_cx;
            g_paint.prev_draw_y = rel_cy;
        } else {
            g_paint.is_drawing = false;
            g_paint.prev_draw_x = -1;
            g_paint.prev_draw_y = -1;
        }
    } else {
        g_paint.is_drawing = false;
        g_paint.prev_draw_x = -1;
        g_paint.prev_draw_y = -1;
    }

    /* 1. Top Toolbar Background */
    gfx_fill_rounded_rect(start_x + 8, start_y + 6, win_w - 16, 36, 8, COLOR_ARGB(180, 28, 18, 46));
    gfx_draw_rounded_rect(start_x + 8, start_y + 6, win_w - 16, 36, 8, COLOR_ARGB(90, 255, 182, 193), 1);

    /* 2. Color Palette Swatches */
    int pal_start_x = start_x + 16;
    int pal_y = start_y + 12;
    int swatch_size = 22;
    int gap = 4;

    for (int i = 0; i < PALETTE_COUNT; i++) {
        int sx = pal_start_x + i * (swatch_size + gap);
        bool selected = (g_paint.current_color == g_palette[i]);

        gfx_fill_rounded_rect(sx, pal_y, swatch_size, swatch_size, 4, g_palette[i]);
        if (selected) {
            gfx_draw_rounded_rect(sx - 1, pal_y - 1, swatch_size + 2, swatch_size + 2, 5, STELLA_DEEP_PINK, 2);
        } else {
            gfx_draw_rounded_rect(sx, pal_y, swatch_size, swatch_size, 4, COLOR_ARGB(90, 0, 0, 0), 1);
        }
    }

    /* 3. Brush Size Selectors */
    int size_start_x = pal_start_x + PALETTE_COUNT * (swatch_size + gap) + 10;
    const char *size_labels[] = { "1", "3", "5", "8" };

    for (int i = 0; i < BRUSH_SIZES_COUNT; i++) {
        int bx = size_start_x + i * 26;
        bool selected = (g_paint.current_size == g_brush_sizes[i]);
        color_t b_fill = selected ? STELLA_DEEP_PINK : COLOR_ARGB(160, 45, 30, 65);

        gfx_fill_rounded_rect(bx, pal_y, 22, swatch_size, 4, b_fill);
        gfx_draw_rounded_rect(bx, pal_y, 22, swatch_size, 4, COLOR_ARGB(80, 255, 255, 255), 1);
        font_draw_string(bx + 7, pal_y + 3, size_labels[i], STELLA_TEXT_WHITE, 0);
    }

    /* 4. Clear Button */
    int clear_x = size_start_x + BRUSH_SIZES_COUNT * 26 + 10;
    gfx_fill_rounded_rect(clear_x, pal_y, 50, swatch_size, 4, COLOR_ARGB(180, 180, 50, 70));
    gfx_draw_rounded_rect(clear_x, pal_y, 50, swatch_size, 4, COLOR_ARGB(120, 255, 100, 100), 1);
    font_draw_string(clear_x + 6, pal_y + 3, "Clear", STELLA_TEXT_WHITE, 0);

    /* 5. Current Color & Size Preview Indicator */
    int ind_x = clear_x + 58;
    gfx_fill_rounded_rect(ind_x, pal_y, swatch_size, swatch_size, 4, COLOR_RGB(240, 240, 245));
    gfx_draw_rounded_rect(ind_x, pal_y, swatch_size, swatch_size, 4, STELLA_DEEP_PINK, 1);
    int preview_r = g_paint.current_size / 2;
    if (preview_r < 1) preview_r = 1;
    if (preview_r > 7) preview_r = 7;
    gfx_fill_circle(ind_x + swatch_size / 2, pal_y + swatch_size / 2, preview_r, g_paint.current_color);

    /* 6. Render Canvas Buffer */
    gfx_fill_rect(canvas_x - 1, canvas_y - 1, CANVAS_W + 2, CANVAS_H + 2, COLOR_ARGB(120, 200, 160, 255));

    if (g_paint.canvas) {
        for (int cy = 0; cy < CANVAS_H; cy++) {
            for (int cx = 0; cx < CANVAS_W; cx++) {
                color_t p = g_paint.canvas[cy * CANVAS_W + cx];
                fb_putpixel(canvas_x + cx, canvas_y + cy, p);
            }
        }
    }
}

static void paint_click(window_t *win, int cx, int cy) {
    UNUSED(win);
    int pal_start_x = 16;
    int pal_y = 12;
    int swatch_size = 22;
    int gap = 4;

    /* Palette selection */
    for (int i = 0; i < PALETTE_COUNT; i++) {
        int sx = pal_start_x + i * (swatch_size + gap);
        if (cx >= sx && cx < sx + swatch_size && cy >= pal_y && cy < pal_y + swatch_size) {
            g_paint.current_color = g_palette[i];
            return;
        }
    }

    /* Brush size selection */
    int size_start_x = pal_start_x + PALETTE_COUNT * (swatch_size + gap) + 10;
    for (int i = 0; i < BRUSH_SIZES_COUNT; i++) {
        int bx = size_start_x + i * 26;
        if (cx >= bx && cx < bx + 22 && cy >= pal_y && cy < pal_y + swatch_size) {
            g_paint.current_size = g_brush_sizes[i];
            return;
        }
    }

    /* Clear canvas button */
    int clear_x = size_start_x + BRUSH_SIZES_COUNT * 26 + 10;
    if (cx >= clear_x && cx < clear_x + 50 && cy >= pal_y && cy < pal_y + swatch_size) {
        paint_clear_canvas();
        return;
    }
}

static void paint_close(window_t *win) {
    UNUSED(win);
    if (g_paint.canvas) {
        kfree(g_paint.canvas);
        g_paint.canvas = NULL;
    }
    g_paint_win = NULL;
}

void app_paint_open(void) {
    if (g_paint_win) {
        gui_focus_window(g_paint_win);
        return;
    }

    g_paint_win = gui_create_window("StellaPaint *", 120, 50, 520, 420);
    if (!g_paint_win) return;

    g_paint_win->on_draw  = paint_draw;
    g_paint_win->on_click = paint_click;
    g_paint_win->on_close = paint_close;

    g_paint.current_color = COLOR_RGB(255, 105, 180); /* Pink default */
    g_paint.current_size  = 3;
    g_paint.prev_draw_x   = -1;
    g_paint.prev_draw_y   = -1;
    g_paint.is_drawing    = false;

    if (!g_paint.canvas) {
        g_paint.canvas = (color_t *)kmalloc(CANVAS_W * CANVAS_H * sizeof(color_t));
    }
    paint_clear_canvas();
}
