/*
 * StellaOS - editor.c
 * Kawaii desktop code and text editor application
 * Copyright (c) 2024 StellaOS Project
 */

#include "editor.h"
#include "gui.h"
#include "graphics.h"
#include "font.h"
#include "memory.h"
#include "string.h"
#include "keyboard.h"
#include "timer.h"
#include "mouse.h"

#define EDITOR_MAX_CHARS 4096
#define GUTTER_WIDTH     42
#define STATUS_HEIGHT    22
#define LINE_HEIGHT      16

typedef struct {
    char text[EDITOR_MAX_CHARS + 1];
    int length;
    int cursor_pos;
    int scroll_line;
    int sel_start;
    int sel_end;
    char clipboard[1024];
    int clip_len;
    uint32_t last_cursor_tick;
    bool cursor_visible;
} editor_state_t;

static window_t *g_editor_win = NULL;
static editor_state_t g_editor;

static const char *g_default_content =
    "/* StellaEdit v0.1.0 */\n"
    "// Welcome to the kawaii text editor!\n"
    "#include <stella.h>\n"
    "\n"
    "void main(void) {\n"
    "    char *msg = \"Hello, StellaOS! *\";\n"
    "    int starlight_count = 128;\n"
    "    print(msg, starlight_count);\n"
    "}\n";

static void editor_get_cursor_line_col(int *line, int *col) {
    int cur_line = 1;
    int cur_col = 1;
    for (int i = 0; i < g_editor.cursor_pos && i < g_editor.length; i++) {
        if (g_editor.text[i] == '\n') {
            cur_line++;
            cur_col = 1;
        } else {
            cur_col++;
        }
    }
    *line = cur_line;
    *col = cur_col;
}

static void editor_insert_char(char c) {
    if (g_editor.length >= EDITOR_MAX_CHARS) return;

    for (int i = g_editor.length; i > g_editor.cursor_pos; i--) {
        g_editor.text[i] = g_editor.text[i - 1];
    }
    g_editor.text[g_editor.cursor_pos] = c;
    g_editor.cursor_pos++;
    g_editor.length++;
    g_editor.text[g_editor.length] = '\0';
}

static void editor_delete_backward(void) {
    if (g_editor.cursor_pos <= 0 || g_editor.length <= 0) return;

    for (int i = g_editor.cursor_pos - 1; i < g_editor.length - 1; i++) {
        g_editor.text[i] = g_editor.text[i + 1];
    }
    g_editor.cursor_pos--;
    g_editor.length--;
    g_editor.text[g_editor.length] = '\0';
}

static void editor_delete_forward(void) {
    if (g_editor.cursor_pos >= g_editor.length || g_editor.length <= 0) return;

    for (int i = g_editor.cursor_pos; i < g_editor.length - 1; i++) {
        g_editor.text[i] = g_editor.text[i + 1];
    }
    g_editor.length--;
    g_editor.text[g_editor.length] = '\0';
}

static void editor_move_up(void) {
    int cur_line, cur_col;
    editor_get_cursor_line_col(&cur_line, &cur_col);
    if (cur_line <= 1) return;

    /* Find start of current line */
    int cur_line_start = g_editor.cursor_pos;
    while (cur_line_start > 0 && g_editor.text[cur_line_start - 1] != '\n') {
        cur_line_start--;
    }

    /* Find start of previous line */
    int prev_line_end = cur_line_start - 1;
    int prev_line_start = prev_line_end;
    while (prev_line_start > 0 && g_editor.text[prev_line_start - 1] != '\n') {
        prev_line_start--;
    }

    int prev_line_len = prev_line_end - prev_line_start;
    int target_col = (cur_col - 1 < prev_line_len) ? (cur_col - 1) : prev_line_len;
    g_editor.cursor_pos = prev_line_start + target_col;
}

static void editor_move_down(void) {
    int cur_line, cur_col;
    editor_get_cursor_line_col(&cur_line, &cur_col);

    /* Find end of current line */
    int cur_line_end = g_editor.cursor_pos;
    while (cur_line_end < g_editor.length && g_editor.text[cur_line_end] != '\n') {
        cur_line_end++;
    }

    if (cur_line_end >= g_editor.length) return;

    /* Next line start */
    int next_line_start = cur_line_end + 1;
    int next_line_end = next_line_start;
    while (next_line_end < g_editor.length && g_editor.text[next_line_end] != '\n') {
        next_line_end++;
    }

    int next_line_len = next_line_end - next_line_start;
    int target_col = (cur_col - 1 < next_line_len) ? (cur_col - 1) : next_line_len;
    g_editor.cursor_pos = next_line_start + target_col;
}

static void editor_copy_selection(void) {
    if (g_editor.sel_start >= 0 && g_editor.sel_end > g_editor.sel_start) {
        int count = g_editor.sel_end - g_editor.sel_start;
        if (count > (int)sizeof(g_editor.clipboard) - 1) {
            count = sizeof(g_editor.clipboard) - 1;
        }
        memcpy(g_editor.clipboard, &g_editor.text[g_editor.sel_start], count);
        g_editor.clipboard[count] = '\0';
        g_editor.clip_len = count;
    } else {
        /* Copy current line */
        int start = g_editor.cursor_pos;
        while (start > 0 && g_editor.text[start - 1] != '\n') start--;
        int end = g_editor.cursor_pos;
        while (end < g_editor.length && g_editor.text[end] != '\n') end++;

        int count = end - start;
        if (count > (int)sizeof(g_editor.clipboard) - 1) {
            count = sizeof(g_editor.clipboard) - 1;
        }
        memcpy(g_editor.clipboard, &g_editor.text[start], count);
        g_editor.clipboard[count] = '\0';
        g_editor.clip_len = count;
    }
}

static void editor_draw(window_t *win) {
    int start_x = win->x;
    int start_y = win->y + WINDOW_TITLE_HEIGHT;
    int win_w   = win->width;
    int win_h   = win->height - WINDOW_TITLE_HEIGHT;

    int text_area_w = win_w - GUTTER_WIDTH;
    int text_area_h = win_h - STATUS_HEIGHT;

    /* 1. Left Gutter (Line Numbers) */
    gfx_fill_rect(start_x, start_y, GUTTER_WIDTH, text_area_h, COLOR_ARGB(180, 24, 16, 42));
    gfx_fill_rect(start_x + GUTTER_WIDTH - 1, start_y, 1, text_area_h, COLOR_ARGB(90, 200, 160, 255));

    /* 2. Text Editing Area */
    gfx_fill_rect(start_x + GUTTER_WIDTH, start_y, text_area_w, text_area_h, COLOR_ARGB(230, 28, 18, 48));

    /* Cursor blink timing */
    uint32_t ticks = timer_get_ticks();
    if (ticks - g_editor.last_cursor_tick > 30) {
        g_editor.cursor_visible = !g_editor.cursor_visible;
        g_editor.last_cursor_tick = ticks;
    }

    /* Render lines with line numbers and syntax-ish coloring */
    int cur_line = 1;
    int line_start_pos = 0;
    int cursor_pixel_x = -1;
    int cursor_pixel_y = -1;

    int max_visible_lines = text_area_h / LINE_HEIGHT;
    int visible_line_idx = 0;

    int pos = 0;
    while (pos <= g_editor.length && visible_line_idx < max_visible_lines) {
        /* Line number */
        int row_y = start_y + visible_line_idx * LINE_HEIGHT;

        char num_buf[8];
        itoa(cur_line, num_buf, 10);
        int num_w = font_string_width(num_buf);
        font_draw_string(start_x + GUTTER_WIDTH - num_w - 6, row_y, num_buf, COLOR_RGB(160, 130, 190), 0);

        /* Render this line characters */
        int col_x = start_x + GUTTER_WIDTH + 8;
        bool in_string = false;
        bool in_comment = false;

        while (pos < g_editor.length && g_editor.text[pos] != '\n') {
            if (pos == g_editor.cursor_pos) {
                cursor_pixel_x = col_x;
                cursor_pixel_y = row_y;
            }

            char c = g_editor.text[pos];

            /* Syntax highlighting state */
            if (c == '"') {
                in_string = !in_string;
            } else if (c == '/' && pos + 1 < g_editor.length &&
                       (g_editor.text[pos + 1] == '/' || g_editor.text[pos + 1] == '*')) {
                in_comment = true;
            }

            color_t char_color = STELLA_TEXT_WHITE;
            if (in_comment) {
                char_color = COLOR_RGB(100, 255, 180);
            } else if (in_string || c == '"') {
                char_color = STELLA_LAVENDER;
            } else if (c >= '0' && c <= '9') {
                char_color = STELLA_PINK;
            }

            if (col_x + 8 < start_x + win_w - 4) {
                font_draw_char(col_x, row_y, c, char_color, 0);
            }

            col_x += 8;
            pos++;
        }

        if (pos == g_editor.cursor_pos) {
            cursor_pixel_x = col_x;
            cursor_pixel_y = row_y;
        }

        /* Advance to next line */
        if (pos < g_editor.length && g_editor.text[pos] == '\n') {
            pos++;
        }
        cur_line++;
        visible_line_idx++;
        line_start_pos = pos;
        UNUSED(line_start_pos);
    }

    /* 3. Blinking Cursor */
    if (g_editor.cursor_visible && cursor_pixel_x >= 0 && cursor_pixel_y >= 0) {
        gfx_fill_rect(cursor_pixel_x, cursor_pixel_y, 2, LINE_HEIGHT - 2, STELLA_DEEP_PINK);
    }

    /* 4. Bottom Status Bar */
    int stat_y = start_y + text_area_h;
    gfx_fill_rect(start_x, stat_y, win_w, STATUS_HEIGHT, COLOR_ARGB(230, 20, 12, 36));
    gfx_fill_rect(start_x, stat_y, win_w, 1, COLOR_ARGB(90, 255, 182, 193));

    int l_num, c_num;
    editor_get_cursor_line_col(&l_num, &c_num);

    char stat_str[80];
    char l_buf[8], c_buf[8], len_buf[8];
    itoa(l_num, l_buf, 10);
    itoa(c_num, c_buf, 10);
    itoa(g_editor.length, len_buf, 10);

    strcpy(stat_str, "Ln ");
    strcat(stat_str, l_buf);
    strcat(stat_str, ", Col ");
    strcat(stat_str, c_buf);
    strcat(stat_str, " | Chars: ");
    strcat(stat_str, len_buf);
    strcat(stat_str, "/4096 | UTF-8 | StellaEdit *");

    font_draw_string(start_x + 12, stat_y + 4, stat_str, STELLA_TEXT_LIGHT, 0);
}

static void editor_key(window_t *win, char key) {
    UNUSED(win);
    uint8_t sc = keyboard_get_scancode();

    /* Check Ctrl+C */
    if (key == 0x03 || (keyboard_ctrl_pressed() && (key == 'c' || key == 'C'))) {
        editor_copy_selection();
        return;
    }

    /* Arrow Keys and special movement */
    if (sc == KEY_UP || key == '8') {
        editor_move_up();
        g_editor.cursor_visible = true;
        g_editor.last_cursor_tick = timer_get_ticks();
        return;
    }
    if (sc == KEY_DOWN || key == '2') {
        editor_move_down();
        g_editor.cursor_visible = true;
        g_editor.last_cursor_tick = timer_get_ticks();
        return;
    }
    if (sc == KEY_LEFT || key == '4') {
        if (g_editor.cursor_pos > 0) g_editor.cursor_pos--;
        g_editor.cursor_visible = true;
        g_editor.last_cursor_tick = timer_get_ticks();
        return;
    }
    if (sc == KEY_RIGHT || key == '6') {
        if (g_editor.cursor_pos < g_editor.length) g_editor.cursor_pos++;
        g_editor.cursor_visible = true;
        g_editor.last_cursor_tick = timer_get_ticks();
        return;
    }

    if (sc == 0x53) {
        editor_delete_forward();
        g_editor.cursor_visible = true;
        g_editor.last_cursor_tick = timer_get_ticks();
        return;
    }

    if (key == '\b') {
        editor_delete_backward();
    } else if ((uint8_t)key == 0x7F) {
        editor_delete_forward();
    } else if (key == '\n' || key == '\r') {
        editor_insert_char('\n');
    } else if (key == '\t') {
        editor_insert_char(' ');
        editor_insert_char(' ');
        editor_insert_char(' ');
        editor_insert_char(' ');
    } else if ((uint8_t)key >= 32 && (uint8_t)key <= 126) {
        editor_insert_char(key);
    }

    g_editor.cursor_visible = true;
    g_editor.last_cursor_tick = timer_get_ticks();
}

static void editor_click(window_t *win, int cx, int cy) {
    UNUSED(win);
    if (cx < GUTTER_WIDTH || cy >= win->height - WINDOW_TITLE_HEIGHT - STATUS_HEIGHT) {
        return;
    }

    int clicked_line = (cy / LINE_HEIGHT) + 1;
    int clicked_col  = ((cx - GUTTER_WIDTH - 8) / 8) + 1;
    if (clicked_col < 1) clicked_col = 1;

    /* Move cursor close to clicked line/col */
    int line = 1;
    int pos = 0;
    while (pos < g_editor.length && line < clicked_line) {
        if (g_editor.text[pos] == '\n') line++;
        pos++;
    }

    int col = 1;
    while (pos < g_editor.length && g_editor.text[pos] != '\n' && col < clicked_col) {
        pos++;
        col++;
    }

    g_editor.cursor_pos = pos;
    g_editor.cursor_visible = true;
    g_editor.last_cursor_tick = timer_get_ticks();
}

static void editor_close(window_t *win) {
    UNUSED(win);
    g_editor_win = NULL;
}

void app_editor_open(void) {
    if (g_editor_win) {
        gui_focus_window(g_editor_win);
        return;
    }

    g_editor_win = gui_create_window("StellaEdit *", 100, 60, 580, 420);
    if (!g_editor_win) return;

    g_editor_win->on_draw  = editor_draw;
    g_editor_win->on_key   = editor_key;
    g_editor_win->on_click = editor_click;
    g_editor_win->on_close = editor_close;

    memset(&g_editor, 0, sizeof(editor_state_t));
    strncpy(g_editor.text, g_default_content, EDITOR_MAX_CHARS);
    g_editor.length = strlen(g_editor.text);
    g_editor.cursor_pos = g_editor.length;
    g_editor.cursor_visible = true;
    g_editor.sel_start = -1;
    g_editor.sel_end = -1;
}
