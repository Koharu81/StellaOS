/*
 * StellaOS - gui.c
 * Window manager and anime desktop environment implementation
 * Copyright (c) 2024 StellaOS Project
 */

#include "gui.h"
#include "framebuffer.h"
#include "graphics.h"
#include "font.h"
#include "apps.h"
#include "i18n.h"
#include "installer.h"
#include "memory.h"
#include "string.h"
#include "timer.h"
#include "keyboard.h"
#include "mouse.h"
#include "serial.h"
#include "wallpaper.h"
#include "widgets.h"
#include "notification.h"
#include "contextmenu.h"
#include "lockscreen.h"
#include "calculator.h"
#include "sysmon.h"
#include "editor.h"
#include "paint.h"
#include "snake.h"
#include "rtc.h"

#define MAX_STARS 72

typedef struct {
    int x, y;
    uint8_t size;    /* 1: small dot, 2: circle, 3: sparkle */
    uint8_t phase;   /* twinkle animation phase */
    color_t color;
} star_t;

/* Window manager state */
static window_t *g_windows[MAX_WINDOWS];
static int g_window_count = 0;
static int g_next_window_id = 1;

/* Starry background state */
static star_t g_stars[MAX_STARS];
static bool g_stars_initialized = false;

/* Taskbar launcher definitions */
#define LAUNCHER_COUNT 9
typedef struct {
    const char *name;
    const char *badge;
    void (*open_func)(void);
} launcher_item_t;

static const launcher_item_t g_launchers[LAUNCHER_COUNT] = {
    { "Terminal", ">_",  app_terminal_open },
    { "Files",    "F",   app_filemanager_open },
    { "Editor",   "Ed",  app_editor_open },
    { "Paint",    "P",   app_paint_open },
    { "Calc",     "C",   app_calculator_open },
    { "Snake",    "Sn",  app_snake_open },
    { "Monitor",  "M",   app_sysmon_open },
    { "Settings", "S",   app_settings_open },
    { "About",    "*",   app_about_open }
};

/* Forward declarations */
static void init_stars(void);
static void draw_desktop(void);
static void draw_taskbar(int mx, int my);
static void draw_top_bar(void);
static void draw_window(window_t *win);
static void draw_cursor(int mx, int my);

void gui_init(void) {
    g_window_count = 0;
    g_next_window_id = 1;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        g_windows[i] = NULL;
    }
    init_stars();
}

static void init_stars(void) {
    uint32_t sw = fb_get_width();
    uint32_t sh = fb_get_height();
    if (sw == 0 || sh == 0) {
        sw = DEFAULT_SCREEN_WIDTH;
        sh = DEFAULT_SCREEN_HEIGHT;
    }

    /* Seed pseudo-random generator */
    uint32_t seed = 133719;

    for (int i = 0; i < MAX_STARS; i++) {
        seed = seed * 1103515245 + 12345;
        g_stars[i].x = (int)(seed % (sw - 20)) + 10;
        seed = seed * 1103515245 + 12345;
        g_stars[i].y = (int)(seed % (sh - TASKBAR_HEIGHT - 30)) + 10;
        seed = seed * 1103515245 + 12345;
        g_stars[i].size = (uint8_t)((seed % 3) + 1);
        seed = seed * 1103515245 + 12345;
        g_stars[i].phase = (uint8_t)(seed % 64);

        /* Soft pastel colors for anime sparkle aesthetic */
        uint32_t cchoice = (seed >> 8) % 4;
        if (cchoice == 0) {
            g_stars[i].color = STELLA_PINK;
        } else if (cchoice == 1) {
            g_stars[i].color = STELLA_LAVENDER;
        } else if (cchoice == 2) {
            g_stars[i].color = STELLA_SKY_BLUE;
        } else {
            g_stars[i].color = COLOR_RGB(255, 255, 255);
        }
    }
    g_stars_initialized = true;
}

window_t *gui_create_window(const char *title, int x, int y, int w, int h) {
    if (g_window_count >= MAX_WINDOWS) {
        return NULL;
    }

    window_t *win = (window_t *)kmalloc(sizeof(window_t));
    if (!win) {
        return NULL;
    }
    memset(win, 0, sizeof(window_t));

    uint32_t sw = fb_get_width();
    uint32_t sh = fb_get_height();

    /* Clamp initial bounds */
    if (w < 120) w = 120;
    if (h < 80) h = 80;
    if (w > (int)sw) w = sw;
    if (h > (int)sh - TASKBAR_HEIGHT) h = sh - TASKBAR_HEIGHT;

    if (x < 0) x = 20;
    if (y < 0) y = 40;
    if (x + w > (int)sw) x = sw - w - 20;
    if (y + h > (int)sh - TASKBAR_HEIGHT) y = (sh - TASKBAR_HEIGHT) - h - 10;

    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    win->visible = true;
    win->focused = true;
    win->minimized = false;
    win->maximized = false;
    win->dragging = false;
    win->id = g_next_window_id++;

    win->saved_x = x;
    win->saved_y = y;
    win->saved_width = w;
    win->saved_height = h;

    if (title) {
        strncpy(win->title, title, sizeof(win->title) - 1);
    } else {
        strcpy(win->title, "Window");
    }

    int content_h = h > WINDOW_TITLE_HEIGHT ? h - WINDOW_TITLE_HEIGHT : 1;
    win->buffer = (color_t *)kmalloc((size_t)w * content_h * sizeof(color_t));
    if (win->buffer) {
        memset(win->buffer, 0, (size_t)w * content_h * sizeof(color_t));
    }

    /* Unfocus existing windows and add new window to front */
    for (int i = 0; i < g_window_count; i++) {
        if (g_windows[i]) {
            g_windows[i]->focused = false;
        }
    }

    g_windows[g_window_count++] = win;
    return win;
}

void gui_close_window(window_t *win) {
    if (!win) return;

    if (win->on_close) {
        win->on_close(win);
    }

    if (win->buffer) {
        kfree(win->buffer);
        win->buffer = NULL;
    }

    /* Remove from window list */
    int found_idx = -1;
    for (int i = 0; i < g_window_count; i++) {
        if (g_windows[i] == win) {
            found_idx = i;
            break;
        }
    }

    if (found_idx >= 0) {
        for (int i = found_idx; i < g_window_count - 1; i++) {
            g_windows[i] = g_windows[i + 1];
        }
        g_window_count--;
        g_windows[g_window_count] = NULL;
    }

    kfree(win);

    /* Focus new top visible window */
    for (int i = g_window_count - 1; i >= 0; i--) {
        if (g_windows[i] && g_windows[i]->visible && !g_windows[i]->minimized) {
            gui_focus_window(g_windows[i]);
            break;
        }
    }
}

void gui_focus_window(window_t *win) {
    if (!win) return;

    int idx = -1;
    for (int i = 0; i < g_window_count; i++) {
        if (g_windows[i] == win) {
            idx = i;
            break;
        }
    }

    if (idx < 0) return;

    /* Shift windows down and bring win to top */
    for (int i = idx; i < g_window_count - 1; i++) {
        g_windows[i] = g_windows[i + 1];
    }
    g_windows[g_window_count - 1] = win;

    for (int i = 0; i < g_window_count; i++) {
        g_windows[i]->focused = (g_windows[i] == win);
    }
}

window_t *gui_get_focused_window(void) {
    if (g_window_count <= 0) return NULL;
    window_t *top = g_windows[g_window_count - 1];
    if (top && top->visible && !top->minimized) {
        return top;
    }
    return NULL;
}

/* Render anime starry night background */
static void draw_desktop(void) {
    uint32_t sw = fb_get_width();
    uint32_t sh = fb_get_height();

    /* Deep anime twilight gradient */
    color_t top_col = COLOR_RGB(16, 9, 32);
    color_t bot_col = COLOR_RGB(46, 20, 62);
    gfx_fill_gradient_v(0, 0, (int)sw, (int)sh, top_col, bot_col);

    if (!g_stars_initialized) {
        init_stars();
    }

    uint32_t ticks = timer_get_ticks();

    /* Draw twinkling stars and sparkles */
    for (int i = 0; i < MAX_STARS; i++) {
        star_t *st = &g_stars[i];
        uint32_t anim = (ticks / 10 + st->phase) % 64;
        uint8_t alpha = (uint8_t)(140 + (anim > 32 ? (64 - anim) * 3 : anim * 3));
        color_t c = COLOR_ARGB(alpha, COLOR_R(st->color), COLOR_G(st->color), COLOR_B(st->color));

        if (st->size == 1) {
            fb_putpixel(st->x, st->y, c);
        } else if (st->size == 2) {
            gfx_fill_circle(st->x, st->y, 1, c);
        } else {
            /* 4-pointed sparkle star */
            gfx_fill_circle(st->x, st->y, 1, c);
            gfx_draw_line(st->x - 3, st->y, st->x + 3, st->y, c);
            gfx_draw_line(st->x, st->y - 3, st->x, st->y + 3, c);
            fb_putpixel(st->x, st->y, COLOR_RGB(255, 255, 255));
        }
    }
}

/* Render top status bar with clock */
static void draw_top_bar(void) {
    uint32_t sw = fb_get_width();

    /* Top bar badge on left: "StellaOS ★" */
    int brand_w = 140;
    int brand_h = 24;
    int brand_x = 16;
    int brand_y = 8;
    gfx_fill_rounded_rect(brand_x, brand_y, brand_w, brand_h, 12, COLOR_ARGB(160, 35, 22, 54));
    gfx_draw_rounded_rect(brand_x, brand_y, brand_w, brand_h, 12, COLOR_ARGB(80, 255, 182, 193), 1);
    font_draw_string(brand_x + 12, brand_y + 4, "StellaOS *", STELLA_PINK, 0);

    /* Desktop Clock pill on top-right */
    int clock_w = 160;
    int clock_h = 24;
    int clock_x = (int)sw - clock_w - 16;
    int clock_y = 8;

    gfx_draw_shadow(clock_x, clock_y, clock_w, clock_h, 12, 6);
    gfx_fill_rounded_rect(clock_x, clock_y, clock_w, clock_h, 12, COLOR_ARGB(180, 35, 22, 54));
    gfx_draw_rounded_rect(clock_x, clock_y, clock_w, clock_h, 12, COLOR_ARGB(90, 200, 160, 255), 1);

    uint32_t ticks = timer_get_ticks();
    uint32_t total_sec = ticks / 100;
    uint32_t minutes = (12 * 60 + (total_sec / 60)) % (24 * 60);
    uint32_t hrs = minutes / 60;
    uint32_t mins = minutes % 60;

    char time_str[32];
    char h_str[8], m_str[8];
    itoa((int)hrs, h_str, 10);
    itoa((int)mins, m_str, 10);

    strcpy(time_str, "StellaOS * ");
    if (hrs < 10) strcat(time_str, "0");
    strcat(time_str, h_str);
    strcat(time_str, ":");
    if (mins < 10) strcat(time_str, "0");
    strcat(time_str, m_str);

    font_draw_string(clock_x + 10, clock_y + 4, time_str, STELLA_TEXT_LIGHT, 0);
}

/* Render floating taskbar with centered app launcher icons */
static void draw_taskbar(int mx, int my) {
    uint32_t sw = fb_get_width();
    uint32_t sh = fb_get_height();

    int tb_w = 280;
    int tb_h = TASKBAR_HEIGHT;
    int tb_x = ((int)sw - tb_w) / 2;
    int tb_y = (int)sh - tb_h - TASKBAR_MARGIN;

    /* Drop shadow beneath taskbar */
    gfx_draw_shadow(tb_x, tb_y, tb_w, tb_h, TASKBAR_RADIUS, 12);

    /* Taskbar body */
    gfx_fill_rounded_rect(tb_x, tb_y, tb_w, tb_h, TASKBAR_RADIUS, STELLA_TASKBAR_BG);
    gfx_draw_rounded_rect(tb_x, tb_y, tb_w, tb_h, TASKBAR_RADIUS, COLOR_ARGB(120, 200, 160, 255), 1);

    int icon_slot_w = 56;
    int start_x = tb_x + (tb_w - (LAUNCHER_COUNT * icon_slot_w)) / 2;

    for (int i = 0; i < LAUNCHER_COUNT; i++) {
        int ix = start_x + i * icon_slot_w;
        int iy = tb_y + 6;
        int iw = 44;
        int ih = 40;

        bool hover = (mx >= ix && mx < ix + iw && my >= iy && my < iy + ih);
        if (hover) {
            iy -= 2; /* Slight lift on hover */
            gfx_fill_rounded_rect(ix, iy, iw, ih, 10, STELLA_HOVER);
            gfx_draw_rounded_rect(ix, iy, iw, ih, 10, STELLA_DEEP_PINK, 1);
        } else {
            gfx_fill_rounded_rect(ix, iy, iw, ih, 10, COLOR_ARGB(140, 48, 32, 70));
            gfx_draw_rounded_rect(ix, iy, iw, ih, 10, COLOR_ARGB(80, 255, 255, 255), 1);
        }

        /* App icon graphics */
        if (i == 0) {
            /* Terminal icon: ">_" in bright green/pink */
            font_draw_string(ix + 12, iy + 12, ">_", COLOR_RGB(100, 255, 180), 0);
        } else if (i == 1) {
            /* Settings gear icon */
            int gcx = ix + iw / 2;
            int gcy = iy + ih / 2;
            gfx_fill_circle(gcx, gcy, 8, STELLA_LAVENDER);
            gfx_fill_circle(gcx, gcy, 3, COLOR_RGB(48, 32, 70));
            gfx_fill_rect(gcx - 1, gcy - 11, 2, 4, STELLA_LAVENDER);
            gfx_fill_rect(gcx - 1, gcy + 7, 2, 4, STELLA_LAVENDER);
            gfx_fill_rect(gcx - 11, gcy - 1, 4, 2, STELLA_LAVENDER);
            gfx_fill_rect(gcx + 7, gcy - 1, 4, 2, STELLA_LAVENDER);
        } else if (i == 2) {
            /* File Manager folder icon */
            int fx = ix + 10;
            int fy = iy + 11;
            gfx_fill_rounded_rect(fx, fy, 10, 4, 2, COLOR_RGB(255, 215, 130));
            gfx_fill_rounded_rect(fx, fy + 3, 24, 15, 3, COLOR_RGB(255, 190, 80));
            gfx_fill_rounded_rect(fx, fy + 7, 24, 11, 2, COLOR_RGB(255, 225, 140));
        } else if (i == 3) {
            /* About Star icon */
            int scx = ix + iw / 2;
            int scy = iy + ih / 2;
            gfx_fill_circle(scx, scy, 5, STELLA_DEEP_PINK);
            gfx_draw_line(scx, scy - 9, scx, scy + 9, STELLA_PINK);
            gfx_draw_line(scx - 9, scy, scx + 9, scy, STELLA_PINK);
            fb_putpixel(scx, scy, COLOR_RGB(255, 255, 255));
        }

        /* Check if this app window is currently open, draw active dot */
        bool is_open = false;
        for (int w = 0; w < g_window_count; w++) {
            if (g_windows[w] && strstr(g_windows[w]->title, g_launchers[i].name)) {
                is_open = true;
                break;
            }
        }
        if (is_open) {
            gfx_fill_circle(ix + iw / 2, tb_y + tb_h - 5, 2, STELLA_ACCENT);
        }
    }
}

/* Render an individual window with title bar and buttons */
static void draw_window(window_t *win) {
    if (!win || !win->visible || win->minimized) return;

    /* Window shadow underneath */
    gfx_draw_shadow(win->x, win->y, win->width, win->height, WINDOW_BORDER_RADIUS, 12);

    /* Window background */
    gfx_fill_rounded_rect(win->x, win->y, win->width, win->height, WINDOW_BORDER_RADIUS, STELLA_WINDOW_BG);

    /* Title bar */
    color_t title_bg = win->focused ? STELLA_WINDOW_TITLE_BG : COLOR_ARGB(220, 50, 30, 80);
    gfx_fill_rounded_rect(win->x, win->y, win->width, WINDOW_TITLE_HEIGHT + 8, WINDOW_BORDER_RADIUS, title_bg);
    gfx_fill_rect(win->x, win->y + WINDOW_BORDER_RADIUS, win->width, WINDOW_TITLE_HEIGHT - WINDOW_BORDER_RADIUS + 1, title_bg);

    /* Accent line below title bar */
    gfx_fill_rect(win->x, win->y + WINDOW_TITLE_HEIGHT - 1, win->width, 1, COLOR_ARGB(90, 255, 180, 220));

    /* Title text */
    font_draw_string(win->x + 14, win->y + 8, win->title, STELLA_TEXT_WHITE, 0);

    /* Window control buttons (macOS style colored circles on right side) */
    int close_cx = win->x + win->width - 18;
    int max_cx   = win->x + win->width - 36;
    int min_cx   = win->x + win->width - 54;
    int btn_cy   = win->y + WINDOW_TITLE_HEIGHT / 2;
    int btn_r    = 6;

    gfx_fill_circle(close_cx, btn_cy, btn_r, STELLA_CLOSE_BTN);
    gfx_fill_circle(max_cx, btn_cy, btn_r, STELLA_MAXIMIZE_BTN);
    gfx_fill_circle(min_cx, btn_cy, btn_r, STELLA_MINIMIZE_BTN);

    /* Window border outline */
    color_t border_col = win->focused ? COLOR_ARGB(140, 255, 180, 220) : COLOR_ARGB(60, 160, 130, 200);
    gfx_draw_rounded_rect(win->x, win->y, win->width, win->height, WINDOW_BORDER_RADIUS, border_col, 1);

    /* Content buffer blit (if used) */
    int content_w = win->width;
    int content_h = win->height - WINDOW_TITLE_HEIGHT;
    if (win->buffer && content_w > 0 && content_h > 0) {
        int start_x = win->x;
        int start_y = win->y + WINDOW_TITLE_HEIGHT;
        for (int cy = 0; cy < content_h; cy++) {
            for (int cx = 0; cx < content_w; cx++) {
                color_t p = win->buffer[cy * content_w + cx];
                if (COLOR_A(p) > 0) {
                    fb_putpixel(start_x + cx, start_y + cy, p);
                }
            }
        }
    }

    /* Content draw callback */
    if (win->on_draw) {
        win->on_draw(win);
    }
}

/* Custom anime star/sparkle cursor */
static void draw_cursor(int mx, int my) {
    /* 4-pointed glowing sparkle cursor */
    color_t glow = STELLA_DEEP_PINK;
    color_t core = COLOR_RGB(255, 255, 255);

    /* Center core */
    gfx_fill_circle(mx, my, 2, core);

    /* Cardinal rays */
    gfx_draw_line(mx, my - 6, mx, my + 6, glow);
    gfx_draw_line(mx - 6, my, mx + 6, my, glow);

    /* Core cross */
    fb_putpixel(mx, my - 1, core);
    fb_putpixel(mx, my + 1, core);
    fb_putpixel(mx - 1, my, core);
    fb_putpixel(mx + 1, my, core);

    /* Diagonal accent dots */
    fb_putpixel(mx - 2, my - 2, glow);
    fb_putpixel(mx + 2, my - 2, glow);
    fb_putpixel(mx - 2, my + 2, glow);
    fb_putpixel(mx + 2, my + 2, glow);

    /* Trailing sparkle */
    fb_putpixel(mx + 5, my + 5, STELLA_PINK);
}

void gui_main_loop(void) {
    if (installer_is_first_boot()) {
        installer_run();
    }

    int prev_lb = 0;

    while (1) {
        int mx = mouse_get_x();
        int my = mouse_get_y();
        int lb = mouse_left_button();

        /* 1. Draw wallpaper background (replaces old gradient) */
        wallpaper_draw();

        /* 2. Draw desktop widgets */
        if (widgets_visible()) {
            widgets_draw();
        }

        /* 3. Draw all visible windows back to front */
        for (int i = 0; i < g_window_count; i++) {
            if (g_windows[i] && g_windows[i]->visible && !g_windows[i]->minimized) {
                draw_window(g_windows[i]);
            }
        }

        /* 3. Draw taskbar on top */
        draw_taskbar(mx, my);

        /* 4. Draw top bar and clock */
        draw_top_bar();

        /* 5. Draw anime sparkle mouse cursor on very top */
        draw_cursor(mx, my);

        /* 6. Swap buffers */
        fb_swap();

        /* 7. Process mouse events */
        uint32_t sw = fb_get_width();
        uint32_t sh = fb_get_height();
        int tb_w = 280;
        int tb_h = TASKBAR_HEIGHT;
        int tb_x = ((int)sw - tb_w) / 2;
        int tb_y = (int)sh - tb_h - TASKBAR_MARGIN;

        if (lb && !prev_lb) {
            /* Left button just pressed */
            bool handled = false;

            /* Check taskbar clicks */
            if (my >= tb_y && my < tb_y + tb_h && mx >= tb_x && mx < tb_x + tb_w) {
                int icon_slot_w = 56;
                int start_x = tb_x + (tb_w - (LAUNCHER_COUNT * icon_slot_w)) / 2;
                for (int i = 0; i < LAUNCHER_COUNT; i++) {
                    int ix = start_x + i * icon_slot_w;
                    int iy = tb_y + 6;
                    if (mx >= ix && mx < ix + 44 && my >= iy && my < iy + 40) {
                        if (g_launchers[i].open_func) {
                            g_launchers[i].open_func();
                        }
                        handled = true;
                        break;
                    }
                }
            }

            /* Check window clicks (front to back) */
            if (!handled) {
                for (int i = g_window_count - 1; i >= 0; i--) {
                    window_t *w = g_windows[i];
                    if (!w || !w->visible || w->minimized) continue;

                    if (mx >= w->x && mx < w->x + w->width &&
                        my >= w->y && my < w->y + w->height) {

                        gui_focus_window(w);
                        handled = true;

                        if (my < w->y + WINDOW_TITLE_HEIGHT) {
                            /* Title bar buttons */
                            int close_cx = w->x + w->width - 18;
                            int max_cx   = w->x + w->width - 36;
                            int min_cx   = w->x + w->width - 54;
                            int btn_cy   = w->y + WINDOW_TITLE_HEIGHT / 2;

                            int d_close = (mx - close_cx) * (mx - close_cx) + (my - btn_cy) * (my - btn_cy);
                            int d_max   = (mx - max_cx)   * (mx - max_cx)   + (my - btn_cy) * (my - btn_cy);
                            int d_min   = (mx - min_cx)   * (mx - min_cx)   + (my - btn_cy) * (my - btn_cy);

                            if (d_close <= 8 * 8) {
                                gui_close_window(w);
                            } else if (d_max <= 8 * 8) {
                                if (w->maximized) {
                                    w->x = w->saved_x;
                                    w->y = w->saved_y;
                                    w->width = w->saved_width;
                                    w->height = w->saved_height;
                                    w->maximized = false;
                                } else {
                                    w->saved_x = w->x;
                                    w->saved_y = w->y;
                                    w->saved_width = w->width;
                                    w->saved_height = w->height;
                                    w->x = 0;
                                    w->y = 36;
                                    w->width = (int)sw;
                                    w->height = (int)sh - TASKBAR_HEIGHT - 36;
                                    w->maximized = true;
                                }
                            } else if (d_min <= 8 * 8) {
                                w->minimized = true;
                            } else {
                                /* Start window dragging */
                                w->dragging = true;
                                w->drag_offset_x = mx - w->x;
                                w->drag_offset_y = my - w->y;
                            }
                        } else {
                            /* Content area click */
                            if (w->on_click) {
                                w->on_click(w, mx - w->x, my - (w->y + WINDOW_TITLE_HEIGHT));
                            }
                        }
                        break;
                    }
                }
            }
        } else if (lb && prev_lb) {
            /* Dragging in progress */
            for (int i = 0; i < g_window_count; i++) {
                window_t *w = g_windows[i];
                if (w && w->dragging) {
                    w->x = mx - w->drag_offset_x;
                    w->y = my - w->drag_offset_y;

                    /* Clamp window position so title bar stays on screen */
                    if (w->y < 0) w->y = 0;
                    if (w->y > (int)sh - 40) w->y = (int)sh - 40;
                    break;
                }
            }
        } else if (!lb && prev_lb) {
            /* Mouse released */
            for (int i = 0; i < g_window_count; i++) {
                if (g_windows[i]) {
                    g_windows[i]->dragging = false;
                }
            }
        }

        prev_lb = lb;

        /* 8. Process keyboard events */
        while (keyboard_has_key()) {
            char key = keyboard_getchar();
            window_t *focused = gui_get_focused_window();
            if (focused && focused->on_key) {
                focused->on_key(focused, key);
            }
        }

        /* 9. Throttle loop (~60fps) */
        timer_wait(16);
    }
}
