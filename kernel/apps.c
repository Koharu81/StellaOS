/*
 * StellaOS - apps.c
 * Built-in applications: Terminal, Settings, File Manager, About
 * Copyright (c) 2024 StellaOS Project
 */

#include "apps.h"
#include "gui.h"
#include "graphics.h"
#include "font.h"
#include "i18n.h"
#include "framebuffer.h"
#include "memory.h"
#include "string.h"
#include "timer.h"

/* ========================================================================= */
/*                              TERMINAL APP                                 */
/* ========================================================================= */

#define TERM_ROWS 25
#define TERM_COLS 80

typedef struct {
    char lines[TERM_ROWS][TERM_COLS + 1];
    color_t colors[TERM_ROWS];
    int line_count;
    char input[TERM_COLS + 1];
    int input_len;
    uint32_t last_cursor_tick;
    bool cursor_state;
} term_state_t;

static window_t *g_term_win = NULL;
static term_state_t g_term;

static void term_add_line(const char *text, color_t color) {
    if (g_term.line_count < TERM_ROWS) {
        strncpy(g_term.lines[g_term.line_count], text, TERM_COLS);
        g_term.lines[g_term.line_count][TERM_COLS] = '\0';
        g_term.colors[g_term.line_count] = color;
        g_term.line_count++;
    } else {
        /* Scroll lines up */
        for (int i = 0; i < TERM_ROWS - 1; i++) {
            strcpy(g_term.lines[i], g_term.lines[i + 1]);
            g_term.colors[i] = g_term.colors[i + 1];
        }
        strncpy(g_term.lines[TERM_ROWS - 1], text, TERM_COLS);
        g_term.lines[TERM_ROWS - 1][TERM_COLS] = '\0';
        g_term.colors[TERM_ROWS - 1] = color;
    }
}

static void term_execute_cmd(const char *cmd) {
    /* Skip leading spaces */
    while (*cmd == ' ') cmd++;
    if (*cmd == '\0') return;

    /* Echo command line */
    char prompt_echo[TERM_COLS + 1];
    strcpy(prompt_echo, "stella@hoshizora:~$ ");
    strncat(prompt_echo, cmd, TERM_COLS - 22);
    term_add_line(prompt_echo, STELLA_TEXT_LIGHT);

    if (strcmp(cmd, "help") == 0) {
        term_add_line("Available commands:", STELLA_PINK);
        term_add_line("  help        - Show available commands", STELLA_TEXT_WHITE);
        term_add_line("  clear       - Clear terminal screen", STELLA_TEXT_WHITE);
        term_add_line("  about       - Display StellaOS information", STELLA_TEXT_WHITE);
        term_add_line("  sysinfo     - Display system hardware & memory info", STELLA_TEXT_WHITE);
        term_add_line("  lang <code> - Change system language (e.g. 'lang ko')", STELLA_TEXT_WHITE);
        term_add_line("  stella      - Show kawaii Stella mascot ASCII art", STELLA_TEXT_WHITE);
        term_add_line("  neofetch    - Show system logo and information", STELLA_TEXT_WHITE);
        term_add_line("  exit        - Close the terminal", STELLA_TEXT_WHITE);
    } else if (strcmp(cmd, "clear") == 0) {
        g_term.line_count = 0;
        for (int i = 0; i < TERM_ROWS; i++) {
            g_term.lines[i][0] = '\0';
        }
    } else if (strcmp(cmd, "about") == 0) {
        term_add_line("StellaOS - Kawaii Anime x86 32-bit OS", STELLA_DEEP_PINK);
        term_add_line("Codename: Hoshizora | Version: 0.1.0", STELLA_LAVENDER);
        term_add_line("Built with magic, starlight, and pairs of sparkles! *", STELLA_PINK);
    } else if (strcmp(cmd, "sysinfo") == 0) {
        term_add_line("--- StellaOS System Information ---", STELLA_SKY_BLUE);
        term_add_line("CPU Architecture: x86 32-bit Protected Mode", STELLA_TEXT_WHITE);
        term_add_line("Kernel Heap: 16 MB Total (Base 0x00400000)", STELLA_TEXT_WHITE);
        term_add_line("Resolution: 1280x720 32bpp Double-Buffered", STELLA_TEXT_WHITE);
        term_add_line("Compositor: StellaWM with Alpha Blending", STELLA_TEXT_WHITE);

        char lbuf[64];
        strcpy(lbuf, "Active Language: ");
        strcat(lbuf, i18n_get_lang_name());
        strcat(lbuf, " (");
        strcat(lbuf, i18n_get_lang_code());
        strcat(lbuf, ")");
        term_add_line(lbuf, STELLA_PINK);
    } else if (strncmp(cmd, "lang ", 5) == 0) {
        const char *code = cmd + 5;
        while (*code == ' ') code++;
        i18n_set_language(code);
        char msg[64];
        strcpy(msg, "Switched language to: ");
        strcat(msg, i18n_get_lang_name());
        term_add_line(msg, COLOR_RGB(100, 255, 180));
    } else if (strcmp(cmd, "stella") == 0) {
        term_add_line("    (\\_/)   * Hoshizora Stella *", STELLA_DEEP_PINK);
        term_add_line("   (='.'=)  \"Gambatte, Master! (o^▽^o)ノ\"", STELLA_PINK);
        term_add_line("   (\")_(\")  Keep sparkling in the digital sky! ✨", STELLA_LAVENDER);
    } else if (strcmp(cmd, "neofetch") == 0) {
        term_add_line("    /\\_/\\     stella@hoshizora", STELLA_DEEP_PINK);
        term_add_line("   ( o.o )    ----------------", STELLA_PINK);
        term_add_line("    > ^ <     OS: StellaOS 0.1.0 Hoshizora", STELLA_LAVENDER);
        term_add_line("              Host: StellaPC x86_32", STELLA_TEXT_WHITE);
        term_add_line("              Kernel: StellaKernel v0.1.0", STELLA_TEXT_WHITE);
        term_add_line("              WM: StellaWM Compositor", STELLA_TEXT_WHITE);
        term_add_line("              Theme: Sakura Dark Purple", STELLA_TEXT_WHITE);
        term_add_line("              Memory: 4MB / 16MB", STELLA_SKY_BLUE);
    } else if (strcmp(cmd, "exit") == 0) {
        if (g_term_win) {
            gui_close_window(g_term_win);
            g_term_win = NULL;
        }
    } else {
        char err[TERM_COLS + 1];
        strcpy(err, "Command not found: '");
        strncat(err, cmd, 32);
        strcat(err, "'. Type 'help'.");
        term_add_line(err, COLOR_RGB(255, 120, 120));
    }
}

static void term_draw(window_t *win) {
    int start_x = win->x + 10;
    int start_y = win->y + WINDOW_TITLE_HEIGHT + 8;
    int view_h = win->height - WINDOW_TITLE_HEIGHT - 32;
    int max_visible_lines = view_h / 16;
    if (max_visible_lines > TERM_ROWS) max_visible_lines = TERM_ROWS;

    int first_line = 0;
    if (g_term.line_count > max_visible_lines) {
        first_line = g_term.line_count - max_visible_lines;
    }

    /* Draw existing lines */
    for (int i = 0; i < max_visible_lines; i++) {
        int idx = first_line + i;
        if (idx >= g_term.line_count) break;
        font_draw_string(start_x, start_y + i * 16, g_term.lines[idx], g_term.colors[idx], 0);
    }

    /* Draw input prompt line at bottom */
    int prompt_y = win->y + win->height - 24;
    font_draw_string(start_x, prompt_y, "stella@hoshizora:~$ ", STELLA_DEEP_PINK, 0);

    int prompt_len = 20 * 8;
    font_draw_string(start_x + prompt_len, prompt_y, g_term.input, STELLA_TEXT_WHITE, 0);

    /* Blinking cursor */
    uint32_t cur_ticks = timer_get_ticks();
    if (cur_ticks - g_term.last_cursor_tick > 30) {
        g_term.cursor_state = !g_term.cursor_state;
        g_term.last_cursor_tick = cur_ticks;
    }

    if (g_term.cursor_state) {
        int cursor_x = start_x + prompt_len + g_term.input_len * 8;
        gfx_fill_rect(cursor_x, prompt_y, 8, 16, COLOR_RGB(100, 255, 180));
    }
}

static void term_key(window_t *win, char key) {
    UNUSED(win);
    if (key == '\n' || key == '\r') {
        term_execute_cmd(g_term.input);
        g_term.input[0] = '\0';
        g_term.input_len = 0;
    } else if (key == '\b' || (uint8_t)key == 0x7F) {
        if (g_term.input_len > 0) {
            g_term.input_len--;
            g_term.input[g_term.input_len] = '\0';
        }
    } else if ((uint8_t)key >= 32 && (uint8_t)key <= 126) {
        if (g_term.input_len < TERM_COLS - 24) {
            g_term.input[g_term.input_len++] = key;
            g_term.input[g_term.input_len] = '\0';
        }
    }
}

static void term_close(window_t *win) {
    UNUSED(win);
    g_term_win = NULL;
}

void app_terminal_open(void) {
    if (g_term_win) {
        gui_focus_window(g_term_win);
        return;
    }

    g_term_win = gui_create_window("Terminal >_", 120, 80, 620, 400);
    if (!g_term_win) return;

    g_term_win->on_draw  = term_draw;
    g_term_win->on_key   = term_key;
    g_term_win->on_close = term_close;

    memset(&g_term, 0, sizeof(term_state_t));
    g_term.cursor_state = true;
    term_add_line("Welcome to StellaOS Terminal v0.1.0 (Hoshizora)", COLOR_RGB(100, 255, 180));
    term_add_line("Type 'help' for commands or 'neofetch' for system overview! *", STELLA_PINK);
}

/* ========================================================================= */
/*                              SETTINGS APP                                 */
/* ========================================================================= */

typedef enum {
    SETTINGS_TAB_LANG = 0,
    SETTINGS_TAB_THEME,
    SETTINGS_TAB_ABOUT
} settings_tab_t;

static window_t *g_settings_win = NULL;
static settings_tab_t g_settings_tab = SETTINGS_TAB_LANG;
static int g_settings_lang_page = 0;
#define LANGS_PER_PAGE 7

static void settings_draw(window_t *win) {
    int cx = win->x;
    int cy = win->y + WINDOW_TITLE_HEIGHT;
    int cw = win->width;
    int ch = win->height - WINDOW_TITLE_HEIGHT;

    /* Left sidebar */
    int sb_w = 130;
    gfx_fill_rect(cx, cy, sb_w, ch, COLOR_ARGB(120, 30, 18, 50));
    gfx_fill_rect(cx + sb_w - 1, cy, 1, ch, COLOR_ARGB(80, 200, 160, 255));

    const char *tabs[] = { "Languages", "Appearance", "System Info" };
    for (int t = 0; t < 3; t++) {
        int ty = cy + 16 + t * 40;
        bool active = (g_settings_tab == (settings_tab_t)t);

        if (active) {
            gfx_fill_rounded_rect(cx + 8, ty, sb_w - 16, 32, 8, STELLA_HOVER);
            gfx_draw_rounded_rect(cx + 8, ty, sb_w - 16, 32, 8, STELLA_DEEP_PINK, 1);
            font_draw_string(cx + 18, ty + 8, tabs[t], STELLA_TEXT_WHITE, 0);
        } else {
            font_draw_string(cx + 18, ty + 8, tabs[t], STELLA_TEXT_LIGHT, 0);
        }
    }

    /* Content Area */
    int main_x = cx + sb_w + 16;
    int main_y = cy + 16;

    if (g_settings_tab == SETTINGS_TAB_LANG) {
        font_draw_string(main_x, main_y, "Select Language / Eon-eo Seon-taek", STELLA_PINK, 0);

        int total_langs = i18n_get_language_count();
        int total_pages = (total_langs + LANGS_PER_PAGE - 1) / LANGS_PER_PAGE;
        if (g_settings_lang_page >= total_pages) g_settings_lang_page = total_pages - 1;
        if (g_settings_lang_page < 0) g_settings_lang_page = 0;

        int start_idx = g_settings_lang_page * LANGS_PER_PAGE;

        for (int i = 0; i < LANGS_PER_PAGE; i++) {
            int idx = start_idx + i;
            if (idx >= total_langs) break;

            const language_t *lang = i18n_get_language(idx);
            if (!lang) continue;

            int item_y = main_y + 32 + i * 36;
            bool selected = (strcmp(lang->code, i18n_get_lang_code()) == 0);

            /* Row background */
            if (selected) {
                gfx_fill_rounded_rect(main_x, item_y, 340, 30, 8, COLOR_ARGB(120, 255, 105, 180));
            } else {
                gfx_fill_rounded_rect(main_x, item_y, 340, 30, 8, COLOR_ARGB(80, 45, 25, 70));
            }
            gfx_draw_rounded_rect(main_x, item_y, 340, 30, 8, COLOR_ARGB(60, 255, 255, 255), 1);

            /* Radio button */
            int radio_cx = main_x + 18;
            int radio_cy = item_y + 15;
            gfx_draw_circle(radio_cx, radio_cy, 6, STELLA_TEXT_WHITE);
            if (selected) {
                gfx_fill_circle(radio_cx, radio_cy, 4, STELLA_DEEP_PINK);
            }

            /* Language name and code */
            font_draw_string(main_x + 36, item_y + 7, lang->name, STELLA_TEXT_WHITE, 0);

            char code_badge[16];
            strcpy(code_badge, "[");
            strcat(code_badge, lang->code);
            strcat(code_badge, "]");
            font_draw_string(main_x + 280, item_y + 7, code_badge, STELLA_LAVENDER, 0);
        }

        /* Page navigation buttons */
        int nav_y = cy + ch - 36;
        gfx_fill_rounded_rect(main_x, nav_y, 80, 26, 6, COLOR_ARGB(120, 50, 30, 80));
        gfx_draw_rounded_rect(main_x, nav_y, 80, 26, 6, STELLA_PINK, 1);
        font_draw_string(main_x + 12, nav_y + 5, "< Prev", STELLA_TEXT_WHITE, 0);

        gfx_fill_rounded_rect(main_x + 100, nav_y, 80, 26, 6, COLOR_ARGB(120, 50, 30, 80));
        gfx_draw_rounded_rect(main_x + 100, nav_y, 80, 26, 6, STELLA_PINK, 1);
        font_draw_string(main_x + 112, nav_y + 5, "Next >", STELLA_TEXT_WHITE, 0);

        char pstr[32];
        strcpy(pstr, "Page ");
        char pnum[8];
        itoa(g_settings_lang_page + 1, pnum, 10);
        strcat(pstr, pnum);
        strcat(pstr, " / ");
        itoa(total_pages, pnum, 10);
        strcat(pstr, pnum);
        font_draw_string(main_x + 210, nav_y + 5, pstr, STELLA_TEXT_LIGHT, 0);

    } else if (g_settings_tab == SETTINGS_TAB_THEME) {
        font_draw_string(main_x, main_y, "Theme: Sakura Hoshizora (Anime Aesthetic)", STELLA_DEEP_PINK, 0);
        font_draw_string(main_x, main_y + 20, "Kawaii Color Palette Preview:", STELLA_TEXT_LIGHT, 0);

        typedef struct {
            const char *name;
            color_t color;
        } swatch_t;

        swatch_t swatches[] = {
            { "Stella Pink",    STELLA_PINK },
            { "Deep Pink",      STELLA_DEEP_PINK },
            { "Sakura Light",   STELLA_SAKURA },
            { "Lavender Dream", STELLA_LAVENDER },
            { "Sky Blue",       STELLA_SKY_BLUE },
            { "Dark Twilight",  STELLA_DARK_BG }
        };

        for (int i = 0; i < 6; i++) {
            int row_y = main_y + 50 + i * 38;
            gfx_fill_rounded_rect(main_x, row_y, 36, 26, 6, swatches[i].color);
            gfx_draw_rounded_rect(main_x, row_y, 36, 26, 6, COLOR_RGB(255, 255, 255), 1);
            font_draw_string(main_x + 50, row_y + 6, swatches[i].name, STELLA_TEXT_WHITE, 0);
        }

    } else {
        /* System Info */
        font_draw_string(main_x, main_y, "StellaOS System Information", STELLA_SKY_BLUE, 0);
        gfx_fill_rect(main_x, main_y + 20, 320, 1, COLOR_ARGB(90, 200, 160, 255));

        int sy = main_y + 35;
        font_draw_string(main_x, sy,      "Version:   StellaOS v0.1.0 Hoshizora", STELLA_TEXT_WHITE, 0);
        font_draw_string(main_x, sy + 25, "Kernel:    x86 32-bit Protected Mode", STELLA_TEXT_WHITE, 0);
        font_draw_string(main_x, sy + 50, "Display:   1280x720 True Color (32bpp)", STELLA_TEXT_WHITE, 0);
        font_draw_string(main_x, sy + 75, "Memory:    16MB Dedicated Kernel Heap", STELLA_TEXT_WHITE, 0);
        font_draw_string(main_x, sy + 100, "License:   MIT Open Source License", STELLA_TEXT_WHITE, 0);
        font_draw_string(main_x, sy + 130, "Made with love for anime lovers everywhere! *", STELLA_PINK, 0);
    }
}

static void settings_click(window_t *win, int cx, int cy) {
    int sb_w = 130;
    if (cx < sb_w) {
        /* Clicked sidebar tab */
        for (int t = 0; t < 3; t++) {
            int ty = 16 + t * 40;
            if (cy >= ty && cy < ty + 32) {
                g_settings_tab = (settings_tab_t)t;
                break;
            }
        }
    } else {
        /* Clicked content area */
        int main_x = sb_w + 16;
        int ch = win->height - WINDOW_TITLE_HEIGHT;

        if (g_settings_tab == SETTINGS_TAB_LANG) {
            /* Check language selection */
            int start_idx = g_settings_lang_page * LANGS_PER_PAGE;
            int total_langs = i18n_get_language_count();

            for (int i = 0; i < LANGS_PER_PAGE; i++) {
                int idx = start_idx + i;
                if (idx >= total_langs) break;

                int item_y = 16 + 32 + i * 36;
                if (cx >= main_x && cx < main_x + 340 && cy >= item_y && cy < item_y + 30) {
                    const language_t *lang = i18n_get_language(idx);
                    if (lang) {
                        i18n_set_language(lang->code);
                    }
                    break;
                }
            }

            /* Check pagination */
            int nav_y = ch - 36;
            if (cy >= nav_y && cy < nav_y + 26) {
                if (cx >= main_x && cx < main_x + 80) {
                    if (g_settings_lang_page > 0) g_settings_lang_page--;
                } else if (cx >= main_x + 100 && cx < main_x + 180) {
                    int total_pages = (total_langs + LANGS_PER_PAGE - 1) / LANGS_PER_PAGE;
                    if (g_settings_lang_page < total_pages - 1) g_settings_lang_page++;
                }
            }
        }
    }
}

static void settings_close(window_t *win) {
    UNUSED(win);
    g_settings_win = NULL;
}

void app_settings_open(void) {
    if (g_settings_win) {
        gui_focus_window(g_settings_win);
        return;
    }

    g_settings_win = gui_create_window("Settings *S*", 160, 100, 540, 380);
    if (!g_settings_win) return;

    g_settings_win->on_draw  = settings_draw;
    g_settings_win->on_click = settings_click;
    g_settings_win->on_close = settings_close;
}

/* ========================================================================= */
/*                           FILE MANAGER APP                                */
/* ========================================================================= */

typedef struct {
    const char *name;
    const char *size;
    bool is_folder;
} mock_file_t;

static const mock_file_t g_files[] = {
    { "Documents",        "4 items",  true },
    { "Pictures",         "12 items", true },
    { "Music",            "8 items",  true },
    { "Downloads",        "3 items",  true },
    { "welcome.txt",      "1.2 KB",   false },
    { "stella_theme.mp3", "3.4 MB",   false },
    { "wallpaper.png",    "840 KB",   false },
    { "spells.cfg",       "512 B",    false },
    { "sakura.bmp",       "2.1 MB",   false }
};
#define MOCK_FILE_COUNT ((int)(sizeof(g_files) / sizeof(g_files[0])))

static window_t *g_file_win = NULL;
static int g_selected_file = 0;

static void filemanager_draw(window_t *win) {
    int cx = win->x;
    int cy = win->y + WINDOW_TITLE_HEIGHT;
    int cw = win->width;
    int ch = win->height - WINDOW_TITLE_HEIGHT;

    /* Address bar */
    gfx_fill_rect(cx, cy, cw, 28, COLOR_ARGB(140, 25, 15, 45));
    gfx_fill_rect(cx, cy + 27, cw, 1, COLOR_ARGB(80, 200, 160, 255));
    font_draw_string(cx + 12, cy + 6, "Location: /home/stella/", STELLA_PINK, 0);

    /* Left quick access sidebar */
    int sb_w = 110;
    int sb_y = cy + 28;
    int sb_h = ch - 28 - 24;
    gfx_fill_rect(cx, sb_y, sb_w, sb_h, COLOR_ARGB(100, 30, 20, 50));
    gfx_fill_rect(cx + sb_w - 1, sb_y, 1, sb_h, COLOR_ARGB(60, 200, 160, 255));

    font_draw_string(cx + 8, sb_y + 10, "Favorites", STELLA_LAVENDER, 0);
    font_draw_string(cx + 8, sb_y + 32, " Home", STELLA_TEXT_LIGHT, 0);
    font_draw_string(cx + 8, sb_y + 54, " Desktop", STELLA_TEXT_LIGHT, 0);
    font_draw_string(cx + 8, sb_y + 76, " Storage", STELLA_TEXT_LIGHT, 0);

    /* Main file grid */
    int grid_x = cx + sb_w + 12;
    int grid_y = sb_y + 10;
    int cell_w = 120;
    int cell_h = 70;
    int cols = (cw - sb_w - 24) / cell_w;
    if (cols < 1) cols = 1;

    for (int i = 0; i < MOCK_FILE_COUNT; i++) {
        int r = i / cols;
        int c = i % cols;
        int fx = grid_x + c * cell_w;
        int fy = grid_y + r * cell_h;

        bool sel = (i == g_selected_file);

        if (sel) {
            gfx_fill_rounded_rect(fx, fy, cell_w - 8, cell_h - 6, 8, STELLA_SELECTION);
            gfx_draw_rounded_rect(fx, fy, cell_w - 8, cell_h - 6, 8, STELLA_DEEP_PINK, 1);
        } else {
            gfx_fill_rounded_rect(fx, fy, cell_w - 8, cell_h - 6, 8, COLOR_ARGB(60, 45, 30, 70));
        }

        /* Icon */
        int icx = fx + (cell_w - 8) / 2;
        int icy = fy + 16;
        if (g_files[i].is_folder) {
            /* Folder icon */
            gfx_fill_rounded_rect(icx - 12, icy - 8, 8, 4, 1, COLOR_RGB(255, 215, 130));
            gfx_fill_rounded_rect(icx - 12, icy - 5, 24, 14, 2, COLOR_RGB(255, 190, 80));
            gfx_fill_rounded_rect(icx - 12, icy - 1, 24, 10, 2, COLOR_RGB(255, 225, 140));
        } else {
            /* File document icon */
            gfx_fill_rounded_rect(icx - 9, icy - 10, 18, 22, 2, COLOR_RGB(230, 230, 250));
            gfx_draw_line(icx - 5, icy - 4, icx + 5, icy - 4, COLOR_RGB(120, 120, 160));
            gfx_draw_line(icx - 5, icy,     icx + 5, icy,     COLOR_RGB(120, 120, 160));
            gfx_draw_line(icx - 5, icy + 4, icx + 3, icy + 4, COLOR_RGB(120, 120, 160));
        }

        /* File name */
        int text_w = font_string_width(g_files[i].name);
        int text_x = fx + (cell_w - 8 - text_w) / 2;
        if (text_x < fx + 4) text_x = fx + 4;
        font_draw_string(text_x, fy + 38, g_files[i].name, STELLA_TEXT_WHITE, 0);

        /* Size text */
        int sz_w = font_string_width(g_files[i].size);
        int sz_x = fx + (cell_w - 8 - sz_w) / 2;
        font_draw_string(sz_x, fy + 52, g_files[i].size, STELLA_LAVENDER, 0);
    }

    /* Bottom status bar */
    int stat_y = cy + ch - 24;
    gfx_fill_rect(cx, stat_y, cw, 24, COLOR_ARGB(160, 20, 10, 35));
    gfx_fill_rect(cx, stat_y, cw, 1, COLOR_ARGB(80, 200, 160, 255));

    char status[64];
    strcpy(status, "9 items | Selected: ");
    if (g_selected_file >= 0 && g_selected_file < MOCK_FILE_COUNT) {
        strcat(status, g_files[g_selected_file].name);
    }
    font_draw_string(cx + 12, stat_y + 4, status, STELLA_TEXT_LIGHT, 0);
}

static void filemanager_click(window_t *win, int cx, int cy) {
    int sb_w = 110;
    int top_h = 28;
    int grid_x = sb_w + 12;
    int grid_y = top_h + 10;
    int cell_w = 120;
    int cell_h = 70;
    int cols = (win->width - sb_w - 24) / cell_w;
    if (cols < 1) cols = 1;

    for (int i = 0; i < MOCK_FILE_COUNT; i++) {
        int r = i / cols;
        int c = i % cols;
        int fx = grid_x + c * cell_w;
        int fy = grid_y + r * cell_h;

        if (cx >= fx && cx < fx + cell_w - 8 && cy >= fy && cy < fy + cell_h - 6) {
            g_selected_file = i;
            break;
        }
    }
}

static void filemanager_close(window_t *win) {
    UNUSED(win);
    g_file_win = NULL;
}

void app_filemanager_open(void) {
    if (g_file_win) {
        gui_focus_window(g_file_win);
        return;
    }

    g_file_win = gui_create_window("File Manager 📁", 200, 120, 560, 380);
    if (!g_file_win) return;

    g_file_win->on_draw  = filemanager_draw;
    g_file_win->on_click = filemanager_click;
    g_file_win->on_close = filemanager_close;
}

/* ========================================================================= */
/*                               ABOUT APP                                   */
/* ========================================================================= */

static window_t *g_about_win = NULL;

static void about_draw(window_t *win) {
    int cx = win->x;
    int cy = win->y + WINDOW_TITLE_HEIGHT;
    int cw = win->width;

    /* Center star icon */
    int scx = cx + cw / 2;
    int scy = cy + 40;

    gfx_fill_circle(scx, scy, 10, STELLA_DEEP_PINK);
    gfx_fill_circle(scx, scy, 5, COLOR_RGB(255, 255, 255));
    gfx_draw_line(scx, scy - 18, scx, scy + 18, STELLA_PINK);
    gfx_draw_line(scx - 18, scy, scx + 18, scy, STELLA_PINK);
    gfx_draw_line(scx - 12, scy - 12, scx + 12, scy + 12, STELLA_LAVENDER);
    gfx_draw_line(scx - 12, scy + 12, scx + 12, scy - 12, STELLA_LAVENDER);

    /* Large Title */
    int title_w = font_string_width_scaled("StellaOS", 2);
    font_draw_string_scaled(cx + (cw - title_w) / 2, cy + 70, "StellaOS", STELLA_DEEP_PINK, 0, 2);

    /* Subtitle */
    const char *sub = "Version 0.1.0 Hoshizora (Starry Sky)";
    font_draw_string(cx + (cw - font_string_width(sub)) / 2, cy + 110, sub, STELLA_PINK, 0);

    const char *tagline = "A Magical x86 32-bit Anime Operating System";
    font_draw_string(cx + (cw - font_string_width(tagline)) / 2, cy + 135, tagline, STELLA_TEXT_LIGHT, 0);

    gfx_fill_rect(cx + 40, cy + 160, cw - 80, 1, COLOR_ARGB(80, 200, 160, 255));

    /* Credits & Info */
    font_draw_string(cx + 50, cy + 175, "Made with * by StellaOS Project", STELLA_TEXT_WHITE, 0);
    font_draw_string(cx + 50, cy + 195, "Lead Developer: Koharu & DeepMind Team", STELLA_LAVENDER, 0);
    font_draw_string(cx + 50, cy + 215, "Visual Design:  Hoshizora Studio", STELLA_LAVENDER, 0);
    font_draw_string(cx + 50, cy + 235, "License:        MIT Open Source License", STELLA_LAVENDER, 0);

    /* Close button at bottom */
    int btn_w = 120;
    int btn_h = 30;
    int btn_x = cx + (cw - btn_w) / 2;
    int btn_y = cy + win->height - WINDOW_TITLE_HEIGHT - 45;

    gfx_fill_rounded_rect(btn_x, btn_y, btn_w, btn_h, 8, STELLA_HOVER);
    gfx_draw_rounded_rect(btn_x, btn_y, btn_w, btn_h, 8, STELLA_DEEP_PINK, 1);
    const char *btn_text = "Close *";
    font_draw_string(btn_x + (btn_w - font_string_width(btn_text)) / 2, btn_y + 7, btn_text, STELLA_TEXT_WHITE, 0);
}

static void about_click(window_t *win, int cx, int cy) {
    int cw = win->width;
    int ch = win->height - WINDOW_TITLE_HEIGHT;
    int btn_w = 120;
    int btn_h = 30;
    int btn_x = (cw - btn_w) / 2;
    int btn_y = ch - 45;

    if (cx >= btn_x && cx < btn_x + btn_w && cy >= btn_y && cy < btn_y + btn_h) {
        gui_close_window(win);
        g_about_win = NULL;
    }
}

static void about_close(window_t *win) {
    UNUSED(win);
    g_about_win = NULL;
}

void app_about_open(void) {
    if (g_about_win) {
        gui_focus_window(g_about_win);
        return;
    }

    g_about_win = gui_create_window("About StellaOS *", 220, 140, 460, 340);
    if (!g_about_win) return;

    g_about_win->on_draw  = about_draw;
    g_about_win->on_click = about_click;
    g_about_win->on_close = about_close;
}
