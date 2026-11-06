/*
 * StellaOS - contextmenu.c
 * Kawaii desktop right-click context menu system
 * Copyright (c) 2024 StellaOS Project
 */

#include "contextmenu.h"
#include "graphics.h"
#include "font.h"
#include "framebuffer.h"
#include "string.h"
#include "mouse.h"

#define ITEM_HEIGHT 24
#define SEP_HEIGHT  8
#define MENU_PADDING 6

typedef struct {
    int x, y;
    int width, height;
    bool visible;
    int item_count;
    context_menu_item_t items[CONTEXTMENU_MAX_ITEMS];
} context_menu_state_t;

static context_menu_state_t g_menu;

void contextmenu_init(void) {
    memset(&g_menu, 0, sizeof(context_menu_state_t));
    g_menu.visible = false;
}

void contextmenu_show(int x, int y, const char **items, int count, void (**callbacks)(void)) {
    if (!items || count <= 0) return;
    if (count > CONTEXTMENU_MAX_ITEMS) count = CONTEXTMENU_MAX_ITEMS;

    g_menu.item_count = count;
    int max_label_w = 80;
    int total_h = MENU_PADDING * 2;

    for (int i = 0; i < count; i++) {
        context_menu_item_t *item = &g_menu.items[i];
        const char *label = items[i];

        if (!label || label[0] == '\0' || strcmp(label, "---") == 0 || label[0] == '-') {
            item->label[0] = '\0';
            item->callback = NULL;
            item->enabled = false;
            item->is_separator = true;
            total_h += SEP_HEIGHT;
        } else {
            strncpy(item->label, label, sizeof(item->label) - 1);
            item->label[sizeof(item->label) - 1] = '\0';
            item->callback = callbacks ? callbacks[i] : NULL;
            item->enabled = true;
            item->is_separator = false;

            int lw = font_string_width(item->label);
            if (lw > max_label_w) max_label_w = lw;
            total_h += ITEM_HEIGHT;
        }
    }

    g_menu.width = max_label_w + 36;
    g_menu.height = total_h;

    uint32_t sw = fb_get_width();
    uint32_t sh = fb_get_height();
    if (sw == 0) sw = DEFAULT_SCREEN_WIDTH;
    if (sh == 0) sh = DEFAULT_SCREEN_HEIGHT;

    if (x + g_menu.width > (int)sw - 8) {
        x = (int)sw - g_menu.width - 8;
    }
    if (y + g_menu.height > (int)sh - TASKBAR_HEIGHT - 8) {
        y = (int)sh - TASKBAR_HEIGHT - g_menu.height - 8;
    }
    if (x < 8) x = 8;
    if (y < 8) y = 8;

    g_menu.x = x;
    g_menu.y = y;
    g_menu.visible = true;
}

void contextmenu_hide(void) {
    g_menu.visible = false;
}

bool contextmenu_is_visible(void) {
    return g_menu.visible;
}

bool contextmenu_handle_click(int mx, int my) {
    if (!g_menu.visible) return false;

    /* Check if click was inside menu */
    if (mx >= g_menu.x && mx < g_menu.x + g_menu.width &&
        my >= g_menu.y && my < g_menu.y + g_menu.height) {

        int cur_y = g_menu.y + MENU_PADDING;
        for (int i = 0; i < g_menu.item_count; i++) {
            context_menu_item_t *item = &g_menu.items[i];
            int ih = item->is_separator ? SEP_HEIGHT : ITEM_HEIGHT;

            if (my >= cur_y && my < cur_y + ih) {
                if (item->enabled && !item->is_separator) {
                    void (*cb)(void) = item->callback;
                    contextmenu_hide();
                    if (cb) {
                        cb();
                    }
                    return true;
                }
                break;
            }
            cur_y += ih;
        }
        return true;
    }

    /* Click was outside: dismiss menu and consume the click */
    contextmenu_hide();
    return true;
}

void contextmenu_draw(void) {
    if (!g_menu.visible) return;

    int mx = mouse_get_x();
    int my = mouse_get_y();

    /* 1. Drop Shadow */
    gfx_draw_shadow(g_menu.x, g_menu.y, g_menu.width, g_menu.height, 8, 10);

    /* 2. Menu Background Card */
    gfx_fill_rounded_rect(g_menu.x, g_menu.y, g_menu.width, g_menu.height, 8, COLOR_ARGB(235, 30, 20, 50));
    gfx_draw_rounded_rect(g_menu.x, g_menu.y, g_menu.width, g_menu.height, 8, COLOR_ARGB(120, 255, 182, 193), 1);

    /* 3. Items */
    int cur_y = g_menu.y + MENU_PADDING;
    for (int i = 0; i < g_menu.item_count; i++) {
        context_menu_item_t *item = &g_menu.items[i];

        if (item->is_separator) {
            gfx_fill_rect(g_menu.x + 8, cur_y + SEP_HEIGHT / 2, g_menu.width - 16, 1, COLOR_ARGB(70, 200, 160, 255));
            cur_y += SEP_HEIGHT;
        } else {
            bool hover = (mx >= g_menu.x + 4 && mx < g_menu.x + g_menu.width - 4 &&
                          my >= cur_y && my < cur_y + ITEM_HEIGHT);

            if (hover && item->enabled) {
                gfx_fill_rounded_rect(g_menu.x + 4, cur_y, g_menu.width - 8, ITEM_HEIGHT, 6, STELLA_HOVER);
                gfx_draw_rounded_rect(g_menu.x + 4, cur_y, g_menu.width - 8, ITEM_HEIGHT, 6, STELLA_DEEP_PINK, 1);
                font_draw_string(g_menu.x + 14, cur_y + 4, item->label, COLOR_RGB(255, 255, 255), 0);
            } else {
                color_t tc = item->enabled ? STELLA_TEXT_WHITE : COLOR_RGB(130, 110, 150);
                font_draw_string(g_menu.x + 14, cur_y + 4, item->label, tc, 0);
            }
            cur_y += ITEM_HEIGHT;
        }
    }
}
