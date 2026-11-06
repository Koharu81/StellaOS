/*
 * StellaOS - lockscreen.c
 * Kawaii anime lock screen and login subsystem
 * Copyright (c) 2024 StellaOS Project
 */

#include "lockscreen.h"
#include "framebuffer.h"
#include "graphics.h"
#include "font.h"
#include "timer.h"
#include "string.h"

#define LOCK_SPARKLES 36

typedef struct {
    int x, y;
    uint8_t phase;
    color_t color;
} lock_sparkle_t;

static bool g_is_locked = false;
static char g_lock_pass[32];
static int g_lock_pass_len = 0;
static lock_sparkle_t g_sparkles[LOCK_SPARKLES];
static bool g_sparkles_init = false;

static void init_lock_sparkles(uint32_t sw, uint32_t sh) {
    uint32_t seed = 918273;
    for (int i = 0; i < LOCK_SPARKLES; i++) {
        seed = seed * 1103515245 + 12345;
        g_sparkles[i].x = (int)(seed % sw);
        seed = seed * 1103515245 + 12345;
        g_sparkles[i].y = (int)(seed % sh);
        seed = seed * 1103515245 + 12345;
        g_sparkles[i].phase = (uint8_t)(seed % 32);

        int c = (int)((seed >> 6) % 3);
        if (c == 0) g_sparkles[i].color = STELLA_PINK;
        else if (c == 1) g_sparkles[i].color = STELLA_LAVENDER;
        else g_sparkles[i].color = COLOR_RGB(255, 255, 255);
    }
    g_sparkles_init = true;
}

void lockscreen_init(void) {
    g_is_locked = false;
    g_lock_pass_len = 0;
    g_lock_pass[0] = '\0';
    g_sparkles_init = false;
}

void lockscreen_show(void) {
    g_is_locked = true;
    g_lock_pass_len = 0;
    g_lock_pass[0] = '\0';
}

bool lockscreen_is_locked(void) {
    return g_is_locked;
}

void lockscreen_draw(void) {
    if (!g_is_locked) return;

    uint32_t sw = fb_get_width();
    uint32_t sh = fb_get_height();
    if (sw == 0) sw = DEFAULT_SCREEN_WIDTH;
    if (sh == 0) sh = DEFAULT_SCREEN_HEIGHT;

    if (!g_sparkles_init) {
        init_lock_sparkles(sw, sh);
    }

    uint32_t ticks = timer_get_ticks();

    /* 1. Deep Cosmic Twilight Gradient */
    gfx_fill_gradient_v(0, 0, (int)sw, (int)sh, COLOR_RGB(14, 6, 28), COLOR_RGB(52, 16, 60));

    /* 2. Twinkling Animated Sparkles */
    for (int i = 0; i < LOCK_SPARKLES; i++) {
        lock_sparkle_t *sp = &g_sparkles[i];
        int anim = (int)((ticks / 8 + sp->phase) % 32);
        uint8_t alpha = (uint8_t)(100 + (anim > 16 ? (32 - anim) * 9 : anim * 9));
        color_t col = COLOR_ARGB(alpha, COLOR_R(sp->color), COLOR_G(sp->color), COLOR_B(sp->color));

        gfx_draw_line(sp->x - 3, sp->y, sp->x + 3, sp->y, col);
        gfx_draw_line(sp->x, sp->y - 3, sp->x, sp->y + 3, col);
        fb_putpixel(sp->x, sp->y, COLOR_RGB(255, 255, 255));
    }

    int cx = (int)sw / 2;

    /* 3. OS Title: "StellaOS" scaled with stars */
    const char *title_str = "StellaOS";
    int title_scale = 2;
    int tw = font_string_width_scaled(title_str, title_scale);
    int ty = (int)sh / 7;

    /* Flanking star decorations */
    int star_l = cx - tw / 2 - 28;
    int star_r = cx + tw / 2 + 20;
    font_draw_string(star_l, ty + 6, "*", STELLA_DEEP_PINK, 0);
    font_draw_string(star_r, ty + 6, "*", STELLA_DEEP_PINK, 0);

    font_draw_string_scaled(cx - tw / 2, ty, title_str, STELLA_PINK, 0, title_scale);

    /* 4. Large Centered Time Display */
    uint32_t total_sec = ticks / 100;
    uint32_t minutes = (12 * 60 + (total_sec / 60)) % (24 * 60);
    uint32_t hrs = minutes / 60;
    uint32_t mins = minutes % 60;

    char time_str[16];
    char h_str[8], m_str[8];
    itoa((int)hrs, h_str, 10);
    itoa((int)mins, m_str, 10);

    time_str[0] = '\0';
    if (hrs < 10) strcat(time_str, "0");
    strcat(time_str, h_str);
    strcat(time_str, ":");
    if (mins < 10) strcat(time_str, "0");
    strcat(time_str, m_str);

    int clock_scale = 3;
    int cw = font_string_width_scaled(time_str, clock_scale);
    int clock_y = ty + 50;
    font_draw_string_scaled(cx - cw / 2, clock_y, time_str, COLOR_RGB(255, 255, 255), 0, clock_scale);

    /* Date line */
    const char *date_str = "Thursday, November 5, 2026";
    int dw = font_string_width(date_str);
    font_draw_string(cx - dw / 2, clock_y + 54, date_str, STELLA_SAKURA, 0);

    /* 5. User Profile Card / Avatar */
    int card_y = (int)sh / 2 + 10;
    int avatar_cy = card_y + 26;

    /* Outer glow ring */
    gfx_fill_circle(cx, avatar_cy, 30, COLOR_ARGB(60, 255, 105, 180));
    gfx_fill_circle(cx, avatar_cy, 24, STELLA_DEEP_PINK);
    gfx_fill_circle(cx, avatar_cy, 20, COLOR_ARGB(200, 50, 20, 65));
    font_draw_string(cx - 4, avatar_cy - 7, "*", STELLA_PINK, 0);

    const char *username = "Koharu (Stella)";
    int unw = font_string_width(username);
    font_draw_string(cx - unw / 2, card_y + 60, username, STELLA_TEXT_WHITE, 0);

    /* 6. Password Input Box */
    int pw_w = 220;
    int pw_h = 32;
    int pw_x = cx - pw_w / 2;
    int pw_y = card_y + 84;

    gfx_fill_rounded_rect(pw_x, pw_y, pw_w, pw_h, 8, COLOR_ARGB(180, 30, 16, 48));
    gfx_draw_rounded_rect(pw_x, pw_y, pw_w, pw_h, 8, COLOR_ARGB(140, 255, 182, 193), 1);

    if (g_lock_pass_len == 0) {
        const char *ph = "Enter password...";
        int ph_w = font_string_width(ph);
        font_draw_string(cx - ph_w / 2, pw_y + 8, ph, COLOR_RGB(160, 130, 180), 0);
    } else {
        /* Render bullet stars */
        char bullets[32];
        for (int i = 0; i < g_lock_pass_len && i < 30; i++) {
            bullets[i] = '*';
        }
        bullets[g_lock_pass_len < 30 ? g_lock_pass_len : 30] = '\0';
        int bw = font_string_width(bullets);
        font_draw_string(cx - bw / 2, pw_y + 8, bullets, STELLA_DEEP_PINK, 0);
    }

    /* 7. Bottom Unlock Hint */
    const char *hint = "Press Enter or any key to unlock";
    int hw = font_string_width(hint);
    font_draw_string(cx - hw / 2, pw_y + 44, hint, STELLA_LAVENDER, 0);
}

void lockscreen_handle_key(char key) {
    if (!g_is_locked) return;

    if (key == '\n' || key == '\r' || key == ' ' || key == 27) {
        /* Unlock! */
        g_is_locked = false;
        g_lock_pass_len = 0;
        g_lock_pass[0] = '\0';
        return;
    }

    if (key == '\b' || (uint8_t)key == 0x7F) {
        if (g_lock_pass_len > 0) {
            g_lock_pass_len--;
            g_lock_pass[g_lock_pass_len] = '\0';
        }
        return;
    }

    if ((uint8_t)key >= 32 && (uint8_t)key <= 126) {
        if (g_lock_pass_len < 30) {
            g_lock_pass[g_lock_pass_len++] = key;
            g_lock_pass[g_lock_pass_len] = '\0';
        }
    }
}
