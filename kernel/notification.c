/*
 * StellaOS - notification.c
 * Kawaii toast notification popup subsystem
 * Copyright (c) 2024 StellaOS Project
 */

#include "notification.h"
#include "graphics.h"
#include "font.h"
#include "framebuffer.h"
#include "string.h"
#include "timer.h"

#define TOAST_WIDTH  290
#define TOAST_HEIGHT 54
#define TOAST_GAP    10
#define TOAST_MARGIN 18
#define DEFAULT_DURATION_MS 4000

static notification_t g_notifs[NOTIFICATION_MAX];
static int g_notif_count = 0;

void notification_init(void) {
    memset(g_notifs, 0, sizeof(g_notifs));
    g_notif_count = 0;
}

void notification_show(const char *title, const char *message, int icon_type) {
    if (!title) return;

    /* If list full, discard oldest by shifting left */
    if (g_notif_count >= NOTIFICATION_MAX) {
        for (int i = 0; i < NOTIFICATION_MAX - 1; i++) {
            g_notifs[i] = g_notifs[i + 1];
        }
        g_notif_count = NOTIFICATION_MAX - 1;
    }

    notification_t *n = &g_notifs[g_notif_count];
    strncpy(n->title, title, sizeof(n->title) - 1);
    n->title[sizeof(n->title) - 1] = '\0';

    if (message) {
        strncpy(n->message, message, sizeof(n->message) - 1);
        n->message[sizeof(n->message) - 1] = '\0';
    } else {
        n->message[0] = '\0';
    }

    n->icon_type = icon_type;
    n->duration_ms = DEFAULT_DURATION_MS;
    n->birth_tick = timer_get_ticks();
    n->active = true;

    g_notif_count++;
}

void notification_update(void) {
    if (g_notif_count <= 0) return;

    uint32_t now = timer_get_ticks();
    int i = 0;

    while (i < g_notif_count) {
        notification_t *n = &g_notifs[i];
        uint32_t elapsed_ticks = now - n->birth_tick;
        uint32_t elapsed_ms = elapsed_ticks * 10;

        if (elapsed_ms >= n->duration_ms) {
            /* Remove this notification */
            for (int j = i; j < g_notif_count - 1; j++) {
                g_notifs[j] = g_notifs[j + 1];
            }
            g_notif_count--;
            memset(&g_notifs[g_notif_count], 0, sizeof(notification_t));
        } else {
            i++;
        }
    }
}

void notification_draw(void) {
    if (g_notif_count <= 0) return;

    uint32_t sw = fb_get_width();
    if (sw == 0) sw = DEFAULT_SCREEN_WIDTH;

    uint32_t now = timer_get_ticks();
    int target_x = (int)sw - TOAST_WIDTH - TOAST_MARGIN;
    int base_y   = 44;

    for (int i = 0; i < g_notif_count; i++) {
        notification_t *n = &g_notifs[i];
        if (!n->active) continue;

        /* Slide-in from right animation */
        uint32_t age_ticks = now - n->birth_tick;
        int offset_x = 0;
        if (age_ticks < 12) {
            offset_x = (int)(12 - age_ticks) * 18;
        }

        int tx = target_x + offset_x;
        int ty = base_y + i * (TOAST_HEIGHT + TOAST_GAP);

        /* Icon badge color and symbol */
        color_t border_col = STELLA_PINK;
        const char *badge = "*";
        color_t badge_col = STELLA_PINK;

        if (n->icon_type == NOTIF_INFO) {
            border_col = STELLA_SKY_BLUE;
            badge = "i";
            badge_col = STELLA_SKY_BLUE;
        } else if (n->icon_type == NOTIF_SUCCESS) {
            border_col = COLOR_RGB(100, 255, 180);
            badge = "*";
            badge_col = COLOR_RGB(100, 255, 180);
        } else if (n->icon_type == NOTIF_WARNING) {
            border_col = COLOR_RGB(255, 200, 90);
            badge = "!";
            badge_col = COLOR_RGB(255, 200, 90);
        } else if (n->icon_type == NOTIF_ERROR) {
            border_col = COLOR_RGB(255, 90, 110);
            badge = "x";
            badge_col = COLOR_RGB(255, 90, 110);
        }

        /* 1. Drop Shadow */
        gfx_draw_shadow(tx, ty, TOAST_WIDTH, TOAST_HEIGHT, 10, 8);

        /* 2. Notification Body Card */
        gfx_fill_rounded_rect(tx, ty, TOAST_WIDTH, TOAST_HEIGHT, 10, COLOR_ARGB(235, 30, 20, 50));
        gfx_draw_rounded_rect(tx, ty, TOAST_WIDTH, TOAST_HEIGHT, 10, border_col, 1);

        /* 3. Left Icon Badge */
        int badge_cx = tx + 24;
        int badge_cy = ty + TOAST_HEIGHT / 2;
        gfx_fill_circle(badge_cx, badge_cy, 12, COLOR_ARGB(120, COLOR_R(badge_col), COLOR_G(badge_col), COLOR_B(badge_col)));
        gfx_draw_circle(badge_cx, badge_cy, 12, badge_col);
        font_draw_string(badge_cx - 4, badge_cy - 7, badge, COLOR_RGB(255, 255, 255), 0);

        /* 4. Title and Message text */
        int text_x = tx + 46;
        font_draw_string(text_x, ty + 10, n->title, STELLA_TEXT_WHITE, 0);

        if (n->message[0] != '\0') {
            font_draw_string(text_x, ty + 28, n->message, STELLA_TEXT_LIGHT, 0);
        }
    }
}
