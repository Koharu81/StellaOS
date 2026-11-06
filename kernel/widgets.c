/*
 * StellaOS - widgets.c
 * Aesthetic desktop widgets system
 * Copyright (c) 2024 StellaOS Project
 */

#include "widgets.h"
#include "framebuffer.h"
#include "graphics.h"
#include "font.h"
#include "memory.h"
#include "timer.h"
#include "string.h"

#define WIDGET_COUNT 4

/* Precomputed 60-step sine and cosine table scaled by 100 for analog clock */
static const int8_t g_sin60[60] = {
      0,  10,  21,  31,  41,  50,  59,  67,  74,  81,
     87,  91,  95,  98,  99, 100,  99,  98,  95,  91,
     87,  81,  74,  67,  59,  50,  41,  31,  21,  10,
      0, -10, -21, -31, -41, -50, -59, -67, -74, -81,
    -87, -91, -95, -98, -99,-100, -99, -98, -95, -91,
    -87, -81, -74, -67, -59, -50, -41, -31, -21, -10
};

static const int8_t g_cos60[60] = {
    100,  99,  98,  95,  91,  87,  81,  74,  67,  59,
     50,  41,  31,  21,  10,   0, -10, -21, -31, -41,
    -50, -59, -67, -74, -81, -87, -91, -95, -98, -99,
   -100, -99, -98, -95, -91, -87, -81, -74, -67, -59,
    -50, -41, -31, -21, -10,   0,  10,  21,  31,  41,
     50,  59,  67,  74,  81,  87,  91,  95,  98,  99
};

static bool g_widgets_visible = true;

/* 1. Analog Clock Widget */
static void draw_clock_widget(int x, int y, int w, int h) {
    /* Glass card background */
    gfx_draw_shadow(x, y, w, h, 12, 8);
    gfx_fill_rounded_rect(x, y, w, h, 12, COLOR_ARGB(160, 26, 16, 44));
    gfx_draw_rounded_rect(x, y, w, h, 12, COLOR_ARGB(80, 255, 182, 193), 1);

    int cx = x + w / 2;
    int cy = y + 54;
    int r  = 40;

    /* Clock face */
    gfx_fill_circle(cx, cy, r, COLOR_ARGB(180, 36, 22, 58));
    gfx_draw_circle(cx, cy, r, COLOR_ARGB(120, 200, 160, 255));

    /* 12 Hour tick marks */
    for (int i = 0; i < 12; i++) {
        int idx = (i * 5) % 60;
        int tx = cx + (r * (int)g_sin60[idx]) / 100;
        int ty = cy - (r * (int)g_cos60[idx]) / 100;
        fb_putpixel(tx, ty, STELLA_PINK);
    }

    uint32_t ticks = timer_get_ticks();
    uint32_t total_sec = ticks / 100;
    int sec = (int)(total_sec % 60);
    int min = (int)((total_sec / 60 + 34) % 60);
    int hr  = (int)((total_sec / 3600 + 12) % 12);
    int hr_step = (hr * 5 + min / 12) % 60;

    /* Hour hand */
    int h_len = 20;
    int hx = cx + (h_len * (int)g_sin60[hr_step]) / 100;
    int hy = cy - (h_len * (int)g_cos60[hr_step]) / 100;
    gfx_draw_line(cx, cy, hx, hy, STELLA_DEEP_PINK);

    /* Minute hand */
    int m_len = 30;
    int mx_pt = cx + (m_len * (int)g_sin60[min]) / 100;
    int my_pt = cy - (m_len * (int)g_cos60[min]) / 100;
    gfx_draw_line(cx, cy, mx_pt, my_pt, STELLA_LAVENDER);

    /* Second hand */
    int s_len = 34;
    int sx_pt = cx + (s_len * (int)g_sin60[sec]) / 100;
    int sy_pt = cy - (s_len * (int)g_cos60[sec]) / 100;
    gfx_draw_line(cx, cy, sx_pt, sy_pt, COLOR_RGB(100, 255, 200));

    /* Center pivot */
    gfx_fill_circle(cx, cy, 2, COLOR_RGB(255, 255, 255));

    /* Digital time string below clock */
    char t_str[16];
    char h_str[8], m_str[8];
    itoa((hr == 0 ? 12 : hr), h_str, 10);
    itoa(min, m_str, 10);

    strcpy(t_str, h_str);
    strcat(t_str, ":");
    if (min < 10) strcat(t_str, "0");
    strcat(t_str, m_str);

    int tw = font_string_width(t_str);
    font_draw_string(x + (w - tw) / 2, y + 104, t_str, STELLA_TEXT_WHITE, 0);

    const char *badge = "StellaClock *";
    int bw = font_string_width(badge);
    font_draw_string(x + (w - bw) / 2, y + 124, badge, STELLA_PINK, 0);
}

/* 2. Calendar Widget */
static void draw_calendar_widget(int x, int y, int w, int h) {
    gfx_draw_shadow(x, y, w, h, 12, 8);
    gfx_fill_rounded_rect(x, y, w, h, 12, COLOR_ARGB(160, 26, 16, 44));
    gfx_draw_rounded_rect(x, y, w, h, 12, COLOR_ARGB(80, 255, 182, 193), 1);

    font_draw_string(x + 14, y + 10, "November 2026", STELLA_PINK, 0);

    /* Weekday headers */
    const char *headers = "S  M  T  W  T  F  S";
    font_draw_string(x + 12, y + 28, headers, STELLA_LAVENDER, 0);

    /* 30 days grid (Nov 2026 starts on Sunday) */
    int col_w = 20;
    int row_h = 16;
    int start_y = y + 46;

    for (int day = 1; day <= 30; day++) {
        int d_idx = day - 1; /* Day 1 is Sunday col 0 */
        int col = d_idx % 7;
        int row = d_idx / 7;

        int dx = x + 12 + col * col_w;
        int dy = start_y + row * row_h;

        if (day == 5) {
            /* Today: Highlight with pink circle */
            gfx_fill_circle(dx + 5, dy + 7, 7, STELLA_DEEP_PINK);
            font_draw_string(dx + 2, dy, "5", COLOR_RGB(255, 255, 255), 0);
        } else {
            char d_str[4];
            itoa(day, d_str, 10);
            font_draw_string(dx, dy, d_str, STELLA_TEXT_LIGHT, 0);
        }
    }
}

/* 3. System Info Widget */
static void draw_sysinfo_widget(int x, int y, int w, int h) {
    gfx_draw_shadow(x, y, w, h, 12, 8);
    gfx_fill_rounded_rect(x, y, w, h, 12, COLOR_ARGB(160, 26, 16, 44));
    gfx_draw_rounded_rect(x, y, w, h, 12, COLOR_ARGB(80, 255, 182, 193), 1);

    font_draw_string(x + 12, y + 10, "Stella System *", STELLA_SKY_BLUE, 0);

    /* Memory bar */
    uint32_t mem_total = memory_get_total();
    uint32_t mem_used  = memory_get_used();
    if (mem_total == 0) {
        mem_total = 16 * 1024 * 1024;
        mem_used  = 4 * 1024 * 1024;
    }

    uint32_t t_kb = mem_total / 1024;
    uint32_t u_kb = mem_used / 1024;
    int pct = (t_kb > 0) ? (int)((u_kb * 100) / t_kb) : 25;

    char ram_txt[32];
    char p_buf[8];
    itoa(pct, p_buf, 10);
    strcpy(ram_txt, "RAM: ");
    strcat(ram_txt, p_buf);
    strcat(ram_txt, "% Used");
    font_draw_string(x + 12, y + 30, ram_txt, STELLA_TEXT_WHITE, 0);

    /* Progress bar */
    int pb_w = w - 24;
    gfx_fill_rounded_rect(x + 12, y + 46, pb_w, 8, 3, COLOR_ARGB(140, 20, 10, 34));
    int fill_w = (pb_w * pct) / 100;
    if (fill_w > 0) {
        gfx_fill_rounded_rect(x + 12, y + 46, fill_w, 8, 3, STELLA_DEEP_PINK);
    }
    gfx_draw_rounded_rect(x + 12, y + 46, pb_w, 8, 3, COLOR_ARGB(90, 200, 160, 255), 1);

    /* Uptime */
    uint32_t total_sec = timer_get_ticks() / 100;
    uint32_t hrs = total_sec / 3600;
    uint32_t mins = (total_sec / 60) % 60;

    char up_txt[32];
    char h_str[8], m_str[8];
    itoa((int)hrs, h_str, 10);
    itoa((int)mins, m_str, 10);

    strcpy(up_txt, "Uptime: ");
    strcat(up_txt, h_str);
    strcat(up_txt, "h ");
    strcat(up_txt, m_str);
    strcat(up_txt, "m");
    font_draw_string(x + 12, y + 64, up_txt, STELLA_TEXT_LIGHT, 0);
}

/* 4. Weather Widget */
static void draw_weather_widget(int x, int y, int w, int h) {
    gfx_draw_shadow(x, y, w, h, 12, 8);
    gfx_fill_rounded_rect(x, y, w, h, 12, COLOR_ARGB(160, 26, 16, 44));
    gfx_draw_rounded_rect(x, y, w, h, 12, COLOR_ARGB(80, 255, 182, 193), 1);

    /* Cute Sun Icon with rays */
    int sx = x + 30;
    int sy = y + 32;
    color_t sun_col = COLOR_RGB(255, 215, 60);
    gfx_fill_circle(sx, sy, 14, sun_col);
    gfx_draw_line(sx - 18, sy, sx + 18, sy, sun_col);
    gfx_draw_line(sx, sy - 18, sx, sy + 18, sun_col);

    /* Fluffy Cloud overlapping sun */
    int cx = sx + 10;
    int cy = sy + 8;
    color_t cloud_col = COLOR_RGB(240, 245, 255);
    gfx_fill_circle(cx - 8, cy, 8, cloud_col);
    gfx_fill_circle(cx + 6, cy, 9, cloud_col);
    gfx_fill_circle(cx - 1, cy - 5, 10, cloud_col);
    gfx_fill_rect(cx - 8, cy, 15, 8, cloud_col);

    /* Temperature Text */
    font_draw_string(x + 64, y + 16, "25 C", COLOR_RGB(255, 255, 255), 0);
    font_draw_string(x + 64, y + 34, "Sunny *", STELLA_PINK, 0);
    font_draw_string(x + 64, y + 52, "Tokyo / Star", STELLA_TEXT_LIGHT, 0);
}

static widget_t g_widget_list[WIDGET_COUNT];

void widgets_init(void) {
    uint32_t sw = fb_get_width();
    if (sw == 0) sw = DEFAULT_SCREEN_WIDTH;

    int right_x = (int)sw - 180;

    /* 0: Clock */
    g_widget_list[0].x = right_x;
    g_widget_list[0].y = 44;
    g_widget_list[0].width = 164;
    g_widget_list[0].height = 146;
    g_widget_list[0].draw = draw_clock_widget;

    /* 1: Calendar */
    g_widget_list[1].x = right_x;
    g_widget_list[1].y = 202;
    g_widget_list[1].width = 164;
    g_widget_list[1].height = 142;
    g_widget_list[1].draw = draw_calendar_widget;

    /* 2: System Info */
    g_widget_list[2].x = right_x;
    g_widget_list[2].y = 356;
    g_widget_list[2].width = 164;
    g_widget_list[2].height = 88;
    g_widget_list[2].draw = draw_sysinfo_widget;

    /* 3: Weather */
    g_widget_list[3].x = 20;
    g_widget_list[3].y = 44;
    g_widget_list[3].width = 164;
    g_widget_list[3].height = 76;
    g_widget_list[3].draw = draw_weather_widget;

    g_widgets_visible = true;
}

void widgets_draw(void) {
    if (!g_widgets_visible) return;

    /* Re-evaluate right_x in case screen size changes */
    uint32_t sw = fb_get_width();
    if (sw == 0) sw = DEFAULT_SCREEN_WIDTH;
    int right_x = (int)sw - 180;
    g_widget_list[0].x = right_x;
    g_widget_list[1].x = right_x;
    g_widget_list[2].x = right_x;

    for (int i = 0; i < WIDGET_COUNT; i++) {
        widget_t *w = &g_widget_list[i];
        if (w->draw) {
            w->draw(w->x, w->y, w->width, w->height);
        }
    }
}

void widgets_toggle(void) {
    g_widgets_visible = !g_widgets_visible;
}

bool widgets_visible(void) {
    return g_widgets_visible;
}
