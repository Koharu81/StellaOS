/*
 * StellaOS - installer.c
 * OS Installer wizard implementation with kawaii anime aesthetic
 * Copyright (c) 2024 StellaOS Project
 */

#include "installer.h"
#include "framebuffer.h"
#include "graphics.h"
#include "font.h"
#include "i18n.h"
#include "keyboard.h"
#include "mouse.h"
#include "timer.h"
#include "string.h"

#define STEP_WELCOME     0
#define STEP_LANGUAGE    1
#define STEP_TIMEZONE    2
#define STEP_USERSETUP   3
#define STEP_PROGRESS    4
#define STEP_COMPLETE    5
#define TOTAL_STEPS      6

static bool g_first_boot = true;
static int  g_step = STEP_WELCOME;

/* Wizard user configuration */
static char g_username[32] = "stella";
static char g_hostname[32] = "hoshizora";
static int  g_active_input_field = 0; /* 0: username, 1: hostname */

/* Timezone choices */
typedef struct {
    const char *label;
    const char *offset;
} tz_option_t;

static const tz_option_t g_timezones[] = {
    { "Asia/Seoul, Tokyo",    "KST/JST +09:00" },
    { "UTC Universal Time",   "UTC +00:00" },
    { "America/New_York",     "EST -05:00" },
    { "America/Los_Angeles",  "PST -08:00" },
    { "Europe/London",        "GMT +00:00" },
    { "Europe/Paris, Berlin", "CET +01:00" },
    { "Asia/Shanghai, Taipei","CST +08:00" },
    { "Australia/Sydney",     "AEST +10:00" }
};
#define TZ_COUNT ((int)(sizeof(g_timezones) / sizeof(g_timezones[0])))
static int g_selected_tz = 0;

/* Language page in installer */
static int g_installer_lang_page = 0;
#define INSTALLER_LANGS_PER_PAGE 8

/* Progress state */
static int g_progress = 0;

void installer_init(void) {
    g_first_boot = true;
    g_step = STEP_WELCOME;
    strcpy(g_username, "stella");
    strcpy(g_hostname, "hoshizora");
    g_active_input_field = 0;
    g_selected_tz = 0;
    g_progress = 0;
}

bool installer_is_first_boot(void) {
    return g_first_boot;
}

/* Helper to draw decorative anime sparkle */
static void draw_sparkle(int x, int y, color_t col) {
    gfx_fill_circle(x, y, 2, col);
    gfx_draw_line(x - 5, y, x + 5, y, col);
    gfx_draw_line(x, y - 5, x, y + 5, col);
    fb_putpixel(x, y, COLOR_RGB(255, 255, 255));
}

/* Helper to draw page indicator dots */
static void draw_page_dots(int cx, int cy) {
    int spacing = 18;
    int start_x = cx - ((TOTAL_STEPS - 1) * spacing) / 2;

    for (int i = 0; i < TOTAL_STEPS; i++) {
        int dot_x = start_x + i * spacing;
        if (i == g_step) {
            gfx_fill_circle(dot_x, cy, 5, STELLA_DEEP_PINK);
            gfx_fill_circle(dot_x, cy, 2, COLOR_RGB(255, 255, 255));
        } else if (i < g_step) {
            gfx_fill_circle(dot_x, cy, 4, STELLA_PINK);
        } else {
            gfx_fill_circle(dot_x, cy, 3, COLOR_ARGB(120, 100, 70, 140));
        }
    }
}

void installer_run(void) {
    uint32_t sw = fb_get_width();
    uint32_t sh = fb_get_height();
    if (sw == 0 || sh == 0) {
        sw = DEFAULT_SCREEN_WIDTH;
        sh = DEFAULT_SCREEN_HEIGHT;
    }

    int card_w = 640;
    int card_h = 460;
    int card_x = ((int)sw - card_w) / 2;
    int card_y = ((int)sh - card_h) / 2;

    int prev_lb = 0;
    bool running = true;

    while (running) {
        int mx = mouse_get_x();
        int my = mouse_get_y();
        int lb = mouse_left_button();

        /* Background gradient */
        gfx_fill_gradient_v(0, 0, (int)sw, (int)sh, COLOR_RGB(15, 8, 28), COLOR_RGB(42, 18, 58));

        /* Decorative stars around background */
        draw_sparkle(card_x - 30, card_y + 40, STELLA_PINK);
        draw_sparkle(card_x - 40, card_y + 220, STELLA_LAVENDER);
        draw_sparkle(card_x - 25, card_y + 380, STELLA_SKY_BLUE);
        draw_sparkle(card_x + card_w + 35, card_y + 60, STELLA_DEEP_PINK);
        draw_sparkle(card_x + card_w + 25, card_y + 250, STELLA_PINK);
        draw_sparkle(card_x + card_w + 40, card_y + 400, STELLA_LAVENDER);

        /* Center card shadow & body */
        gfx_draw_shadow(card_x, card_y, card_w, card_h, 18, 16);
        gfx_fill_rounded_rect(card_x, card_y, card_w, card_h, 18, COLOR_ARGB(235, 32, 20, 50));
        gfx_draw_rounded_rect(card_x, card_y, card_w, card_h, 18, COLOR_ARGB(110, 255, 182, 193), 2);

        /* Top dots indicator */
        draw_page_dots(card_x + card_w / 2, card_y + 24);

        /* Buttons geometry at bottom of card */
        int nav_y = card_y + card_h - 48;
        int btn_w = 110;
        int btn_h = 32;
        int back_btn_x = card_x + 30;
        int next_btn_x = card_x + card_w - btn_w - 30;

        bool hover_back = (mx >= back_btn_x && mx < back_btn_x + btn_w && my >= nav_y && my < nav_y + btn_h);
        bool hover_next = (mx >= next_btn_x && mx < next_btn_x + btn_w && my >= nav_y && my < nav_y + btn_h);

        /* Step-specific rendering */
        if (g_step == STEP_WELCOME) {
            /* 1. Welcome Screen */
            int title_w = font_string_width_scaled("StellaOS *", 2);
            font_draw_string_scaled(card_x + (card_w - title_w) / 2, card_y + 65, "StellaOS *", STELLA_DEEP_PINK, 0, 2);

            const char *sub1 = "Welcome to the Magical Anime Desktop OS";
            font_draw_string(card_x + (card_w - font_string_width(sub1)) / 2, card_y + 115, sub1, STELLA_PINK, 0);

            const char *sub2 = "Fast, lightweight 32-bit protected mode kernel with kawaii charm.";
            font_draw_string(card_x + (card_w - font_string_width(sub2)) / 2, card_y + 140, sub2, STELLA_TEXT_LIGHT, 0);

            /* Cute star mascot art */
            int scx = card_x + card_w / 2;
            int scy = card_y + 220;
            gfx_fill_circle(scx, scy, 24, STELLA_DEEP_PINK);
            gfx_fill_circle(scx, scy, 14, STELLA_PINK);
            gfx_fill_circle(scx, scy, 6, COLOR_RGB(255, 255, 255));
            gfx_draw_line(scx - 45, scy, scx + 45, scy, STELLA_PINK);
            gfx_draw_line(scx, scy - 45, scx, scy + 45, STELLA_PINK);
            gfx_draw_line(scx - 30, scy - 30, scx + 30, scy + 30, STELLA_LAVENDER);
            gfx_draw_line(scx - 30, scy + 30, scx + 30, scy - 30, STELLA_LAVENDER);

            /* Install and Live Mode buttons */
            int action_btn_w = 170;
            int action_btn_h = 38;
            int inst_btn_x = card_x + (card_w - (action_btn_w * 2 + 20)) / 2;
            int live_btn_x = inst_btn_x + action_btn_w + 20;
            int action_btn_y = card_y + card_h - 75;

            bool hover_inst = (mx >= inst_btn_x && mx < inst_btn_x + action_btn_w && my >= action_btn_y && my < action_btn_y + action_btn_h);
            bool hover_live = (mx >= live_btn_x && mx < live_btn_x + action_btn_w && my >= action_btn_y && my < action_btn_y + action_btn_h);

            /* Install button */
            gfx_fill_rounded_rect(inst_btn_x, action_btn_y, action_btn_w, action_btn_h, 10, hover_inst ? STELLA_DEEP_PINK : STELLA_HOVER);
            gfx_draw_rounded_rect(inst_btn_x, action_btn_y, action_btn_w, action_btn_h, 10, STELLA_PINK, 1);
            font_draw_string(inst_btn_x + 18, action_btn_y + 11, "Install StellaOS *", STELLA_TEXT_WHITE, 0);

            /* Try Live button */
            gfx_fill_rounded_rect(live_btn_x, action_btn_y, action_btn_w, action_btn_h, 10, hover_live ? COLOR_ARGB(180, 80, 50, 110) : COLOR_ARGB(120, 50, 30, 75));
            gfx_draw_rounded_rect(live_btn_x, action_btn_y, action_btn_w, action_btn_h, 10, STELLA_LAVENDER, 1);
            font_draw_string(live_btn_x + 28, action_btn_y + 11, "Try Live Mode", STELLA_TEXT_WHITE, 0);

            if (lb && !prev_lb) {
                if (hover_inst) {
                    g_step = STEP_LANGUAGE;
                } else if (hover_live) {
                    g_first_boot = false;
                    running = false;
                }
            }

        } else if (g_step == STEP_LANGUAGE) {
            /* 2. Language Selection */
            font_draw_string(card_x + 35, card_y + 50, i18n_get(STR_SELECT_LANG), STELLA_PINK, 0);
            font_draw_string(card_x + 35, card_y + 70, "Choose your language for the starlight desktop", STELLA_TEXT_LIGHT, 0);

            int total_langs = i18n_get_language_count();
            int total_pages = (total_langs + INSTALLER_LANGS_PER_PAGE - 1) / INSTALLER_LANGS_PER_PAGE;
            int start_idx = g_installer_lang_page * INSTALLER_LANGS_PER_PAGE;

            /* 2 columns x 4 rows */
            int col_w = 270;
            int row_h = 36;
            for (int i = 0; i < INSTALLER_LANGS_PER_PAGE; i++) {
                int idx = start_idx + i;
                if (idx >= total_langs) break;

                const language_t *l = i18n_get_language(idx);
                if (!l) continue;

                int col = i / 4;
                int row = i % 4;
                int lx = card_x + 35 + col * (col_w + 20);
                int ly = card_y + 105 + row * (row_h + 8);

                bool sel = (strcmp(l->code, i18n_get_lang_code()) == 0);
                bool hov = (mx >= lx && mx < lx + col_w && my >= ly && my < ly + row_h);

                if (sel) {
                    gfx_fill_rounded_rect(lx, ly, col_w, row_h, 8, COLOR_ARGB(160, 255, 105, 180));
                    gfx_draw_rounded_rect(lx, ly, col_w, row_h, 8, STELLA_DEEP_PINK, 1);
                } else if (hov) {
                    gfx_fill_rounded_rect(lx, ly, col_w, row_h, 8, COLOR_ARGB(120, 60, 40, 90));
                    gfx_draw_rounded_rect(lx, ly, col_w, row_h, 8, STELLA_PINK, 1);
                } else {
                    gfx_fill_rounded_rect(lx, ly, col_w, row_h, 8, COLOR_ARGB(80, 45, 25, 65));
                }

                /* Radio circle */
                int rcx = lx + 16;
                int rcy = ly + row_h / 2;
                gfx_draw_circle(rcx, rcy, 6, STELLA_TEXT_WHITE);
                if (sel) gfx_fill_circle(rcx, rcy, 4, STELLA_DEEP_PINK);

                font_draw_string(lx + 32, ly + 10, l->name, STELLA_TEXT_WHITE, 0);

                char b[16];
                strcpy(b, "[");
                strcat(b, l->code);
                strcat(b, "]");
                font_draw_string(lx + col_w - font_string_width(b) - 10, ly + 10, b, STELLA_LAVENDER, 0);

                if (lb && !prev_lb && hov) {
                    i18n_set_language(l->code);
                }
            }

            /* Pagination controls */
            int p_y = card_y + 295;
            int pl_btn_x = card_x + 35;
            int pr_btn_x = card_x + 115;
            bool hov_pl = (mx >= pl_btn_x && mx < pl_btn_x + 70 && my >= p_y && my < p_y + 24);
            bool hov_pr = (mx >= pr_btn_x && mx < pr_btn_x + 70 && my >= p_y && my < p_y + 24);

            gfx_fill_rounded_rect(pl_btn_x, p_y, 70, 24, 6, hov_pl ? STELLA_HOVER : COLOR_ARGB(100, 50, 30, 75));
            gfx_draw_rounded_rect(pl_btn_x, p_y, 70, 24, 6, STELLA_PINK, 1);
            font_draw_string(pl_btn_x + 10, p_y + 4, "< Prev", STELLA_TEXT_WHITE, 0);

            gfx_fill_rounded_rect(pr_btn_x, p_y, 70, 24, 6, hov_pr ? STELLA_HOVER : COLOR_ARGB(100, 50, 30, 75));
            gfx_draw_rounded_rect(pr_btn_x, p_y, 70, 24, 6, STELLA_PINK, 1);
            font_draw_string(pr_btn_x + 10, p_y + 4, "Next >", STELLA_TEXT_WHITE, 0);

            if (lb && !prev_lb) {
                if (hov_pl && g_installer_lang_page > 0) g_installer_lang_page--;
                if (hov_pr && g_installer_lang_page < total_pages - 1) g_installer_lang_page++;
            }

        } else if (g_step == STEP_TIMEZONE) {
            /* 3. Timezone Selection */
            font_draw_string(card_x + 35, card_y + 50, i18n_get(STR_SELECT_TIMEZONE), STELLA_PINK, 0);
            font_draw_string(card_x + 35, card_y + 70, "Select your regional time configuration", STELLA_TEXT_LIGHT, 0);

            for (int i = 0; i < TZ_COUNT; i++) {
                int ty = card_y + 100 + i * 35;
                bool sel = (i == g_selected_tz);
                bool hov = (mx >= card_x + 35 && mx < card_x + card_w - 70 && my >= ty && my < ty + 30);

                if (sel) {
                    gfx_fill_rounded_rect(card_x + 35, ty, card_w - 70, 30, 8, COLOR_ARGB(160, 255, 105, 180));
                    gfx_draw_rounded_rect(card_x + 35, ty, card_w - 70, 30, 8, STELLA_DEEP_PINK, 1);
                } else if (hov) {
                    gfx_fill_rounded_rect(card_x + 35, ty, card_w - 70, 30, 8, COLOR_ARGB(120, 60, 40, 90));
                } else {
                    gfx_fill_rounded_rect(card_x + 35, ty, card_w - 70, 30, 8, COLOR_ARGB(80, 45, 25, 65));
                }

                /* Radio circle */
                int rcx = card_x + 52;
                int rcy = ty + 15;
                gfx_draw_circle(rcx, rcy, 6, STELLA_TEXT_WHITE);
                if (sel) gfx_fill_circle(rcx, rcy, 4, STELLA_DEEP_PINK);

                font_draw_string(card_x + 68, ty + 7, g_timezones[i].label, STELLA_TEXT_WHITE, 0);
                font_draw_string(card_x + card_w - 220, ty + 7, g_timezones[i].offset, STELLA_LAVENDER, 0);

                if (lb && !prev_lb && hov) {
                    g_selected_tz = i;
                }
            }

        } else if (g_step == STEP_USERSETUP) {
            /* 4. User Account Setup */
            font_draw_string(card_x + 35, card_y + 50, "User Profile & Hostname", STELLA_PINK, 0);
            font_draw_string(card_x + 35, card_y + 70, "Configure your magical persona and machine identity", STELLA_TEXT_LIGHT, 0);

            /* Username field */
            int f1_y = card_y + 120;
            font_draw_string(card_x + 35, f1_y, i18n_get(STR_USERNAME), STELLA_TEXT_WHITE, 0);
            int box1_y = f1_y + 22;
            bool hov_f1 = (mx >= card_x + 35 && mx < card_x + 380 && my >= box1_y && my < box1_y + 36);
            bool foc_f1 = (g_active_input_field == 0);

            gfx_fill_rounded_rect(card_x + 35, box1_y, 380, 36, 8, COLOR_ARGB(180, 20, 10, 35));
            gfx_draw_rounded_rect(card_x + 35, box1_y, 380, 36, 8, foc_f1 ? STELLA_DEEP_PINK : COLOR_ARGB(80, 200, 160, 255), 1);
            font_draw_string(card_x + 48, box1_y + 10, g_username, STELLA_TEXT_WHITE, 0);

            /* Hostname field */
            int f2_y = card_y + 200;
            font_draw_string(card_x + 35, f2_y, i18n_get(STR_HOSTNAME), STELLA_TEXT_WHITE, 0);
            int box2_y = f2_y + 22;
            bool hov_f2 = (mx >= card_x + 35 && mx < card_x + 380 && my >= box2_y && my < box2_y + 36);
            bool foc_f2 = (g_active_input_field == 1);

            gfx_fill_rounded_rect(card_x + 35, box2_y, 380, 36, 8, COLOR_ARGB(180, 20, 10, 35));
            gfx_draw_rounded_rect(card_x + 35, box2_y, 380, 36, 8, foc_f2 ? STELLA_DEEP_PINK : COLOR_ARGB(80, 200, 160, 255), 1);
            font_draw_string(card_x + 48, box2_y + 10, g_hostname, STELLA_TEXT_WHITE, 0);

            if (lb && !prev_lb) {
                if (hov_f1) g_active_input_field = 0;
                if (hov_f2) g_active_input_field = 1;
            }

            /* Password preview hint */
            font_draw_string(card_x + 35, card_y + 280, "Default root/user password: [none / press enter]", STELLA_LAVENDER, 0);

        } else if (g_step == STEP_PROGRESS) {
            /* 5. Installation Progress */
            font_draw_string(card_x + 35, card_y + 60, i18n_get(STR_INSTALLING), STELLA_PINK, 0);

            const char *status_msg = "Preparing magical storage partitions... *";
            if (g_progress > 20 && g_progress <= 40) {
                status_msg = "Extracting starlight kernel packages... *";
            } else if (g_progress > 40 && g_progress <= 65) {
                status_msg = "Installing kawaii desktop environment... *";
            } else if (g_progress > 65 && g_progress <= 85) {
                status_msg = "Configuring celestial anime themes... *";
            } else if (g_progress > 85) {
                status_msg = "Finalizing dream installation... *";
            }
            font_draw_string(card_x + 35, card_y + 90, status_msg, STELLA_TEXT_LIGHT, 0);

            /* Progress bar track */
            int bar_x = card_x + 35;
            int bar_y = card_y + 170;
            int bar_w = card_w - 70;
            int bar_h = 24;

            gfx_fill_rounded_rect(bar_x, bar_y, bar_w, bar_h, 12, COLOR_ARGB(180, 20, 10, 35));
            gfx_draw_rounded_rect(bar_x, bar_y, bar_w, bar_h, 12, COLOR_ARGB(80, 200, 160, 255), 1);

            int fill_w = (bar_w * g_progress) / 100;
            if (fill_w > 0) {
                gfx_fill_rounded_rect(bar_x, bar_y, fill_w, bar_h, 12, STELLA_DEEP_PINK);
                gfx_fill_circle(bar_x + fill_w - 6, bar_y + bar_h / 2, 4, COLOR_RGB(255, 255, 255));
            }

            char pct_str[16];
            itoa(g_progress, pct_str, 10);
            strcat(pct_str, "%");
            font_draw_string(card_x + (card_w - font_string_width(pct_str)) / 2, bar_y + 36, pct_str, STELLA_TEXT_WHITE, 0);

            /* Animate progress */
            if (g_progress < 100) {
                g_progress++;
            } else {
                g_step = STEP_COMPLETE;
            }

        } else if (g_step == STEP_COMPLETE) {
            /* 6. Installation Complete */
            int title_w = font_string_width_scaled("Complete! *", 2);
            font_draw_string_scaled(card_x + (card_w - title_w) / 2, card_y + 70, "Complete! *", STELLA_DEEP_PINK, 0, 2);

            const char *fin_msg = i18n_get(STR_INSTALL_COMPLETE);
            font_draw_string(card_x + (card_w - font_string_width(fin_msg)) / 2, card_y + 120, fin_msg, STELLA_PINK, 0);

            const char *sub = "StellaOS is ready for your adventures! (o^▽^o)ノ";
            font_draw_string(card_x + (card_w - font_string_width(sub)) / 2, card_y + 150, sub, STELLA_TEXT_LIGHT, 0);

            /* Celebration sparkles */
            int scx = card_x + card_w / 2;
            int scy = card_y + 225;
            gfx_fill_circle(scx, scy, 22, STELLA_DEEP_PINK);
            gfx_fill_circle(scx, scy, 10, COLOR_RGB(255, 255, 255));
            gfx_draw_line(scx - 40, scy, scx + 40, scy, STELLA_PINK);
            gfx_draw_line(scx, scy - 40, scx, scy + 40, STELLA_PINK);

            /* Finish / Reboot button */
            int fin_btn_w = 170;
            int fin_btn_h = 36;
            int fin_btn_x = card_x + (card_w - fin_btn_w) / 2;
            int fin_btn_y = card_y + card_h - 75;

            bool hov_fin = (mx >= fin_btn_x && mx < fin_btn_x + fin_btn_w && my >= fin_btn_y && my < fin_btn_y + fin_btn_h);
            gfx_fill_rounded_rect(fin_btn_x, fin_btn_y, fin_btn_w, fin_btn_h, 10, hov_fin ? STELLA_DEEP_PINK : STELLA_HOVER);
            gfx_draw_rounded_rect(fin_btn_x, fin_btn_y, fin_btn_w, fin_btn_h, 10, STELLA_PINK, 1);
            font_draw_string(fin_btn_x + 18, fin_btn_y + 10, "Start StellaOS *", STELLA_TEXT_WHITE, 0);

            if (lb && !prev_lb && hov_fin) {
                g_first_boot = false;
                running = false;
            }
        }

        /* Draw Back and Next navigation buttons (for steps 1, 2, 3) */
        if (g_step >= STEP_LANGUAGE && g_step <= STEP_USERSETUP) {
            /* Back button */
            gfx_fill_rounded_rect(back_btn_x, nav_y, btn_w, btn_h, 8, hover_back ? COLOR_ARGB(180, 80, 50, 100) : COLOR_ARGB(120, 50, 30, 75));
            gfx_draw_rounded_rect(back_btn_x, nav_y, btn_w, btn_h, 8, STELLA_LAVENDER, 1);
            font_draw_string(back_btn_x + (btn_w - font_string_width(i18n_get(STR_BACK))) / 2, nav_y + 8, i18n_get(STR_BACK), STELLA_TEXT_WHITE, 0);

            /* Next button */
            gfx_fill_rounded_rect(next_btn_x, nav_y, btn_w, btn_h, 8, hover_next ? STELLA_DEEP_PINK : STELLA_HOVER);
            gfx_draw_rounded_rect(next_btn_x, nav_y, btn_w, btn_h, 8, STELLA_PINK, 1);
            font_draw_string(next_btn_x + (btn_w - font_string_width(i18n_get(STR_NEXT))) / 2, nav_y + 8, i18n_get(STR_NEXT), STELLA_TEXT_WHITE, 0);

            if (lb && !prev_lb) {
                if (hover_back) {
                    g_step--;
                } else if (hover_next) {
                    g_step++;
                    if (g_step == STEP_PROGRESS) {
                        g_progress = 0;
                    }
                }
            }
        }

        /* Keyboard input handling during user setup */
        while (keyboard_has_key()) {
            char k = keyboard_getchar();
            if (g_step == STEP_USERSETUP) {
                char *target = (g_active_input_field == 0) ? g_username : g_hostname;
                int len = (int)strlen(target);

                if (k == '\t') {
                    g_active_input_field = 1 - g_active_input_field;
                } else if (k == '\b' || (uint8_t)k == 0x7F) {
                    if (len > 0) {
                        target[len - 1] = '\0';
                    }
                } else if (k == '\n' || k == '\r') {
                    g_step++;
                    if (g_step == STEP_PROGRESS) g_progress = 0;
                } else if ((uint8_t)k >= 32 && (uint8_t)k <= 126 && len < 24) {
                    target[len] = k;
                    target[len + 1] = '\0';
                }
            } else if (g_step >= STEP_LANGUAGE && g_step <= STEP_USERSETUP) {
                if (k == '\n' || k == '\r') {
                    g_step++;
                    if (g_step == STEP_PROGRESS) g_progress = 0;
                } else if (k == 27) { /* Esc */
                    g_step--;
                }
            }
        }

        prev_lb = lb;

        /* Sparkle cursor */
        draw_sparkle(mx, my, STELLA_DEEP_PINK);

        fb_swap();
        timer_wait(16);
    }
}
