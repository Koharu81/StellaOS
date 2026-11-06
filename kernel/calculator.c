/*
 * StellaOS - calculator.c
 * Kawaii themed desktop calculator application
 * Copyright (c) 2024 StellaOS Project
 */

#include "calculator.h"
#include "gui.h"
#include "graphics.h"
#include "font.h"
#include "memory.h"
#include "string.h"
#include "mouse.h"

#define CALC_SCALE 1000LL
#define MAX_TOKENS 16

typedef struct {
    int64_t values[MAX_TOKENS];
    char ops[MAX_TOKENS];
    int token_count;
    char current_input[32];
    char expr_history[64];
    bool new_number;
    bool has_error;
} calc_state_t;

static window_t *g_calc_win = NULL;
static calc_state_t g_calc;

/* Safe 64-bit unsigned division and modulo without libgcc helpers */
static uint64_t udiv64(uint64_t n, uint64_t d) {
    if (d == 0) return 0;
    uint64_t q = 0;
    uint64_t r = 0;
    for (int i = 63; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1);
        if (r >= d) {
            r -= d;
            q |= ((uint64_t)1 << i);
        }
    }
    return q;
}

static uint64_t umod64(uint64_t n, uint64_t d) {
    if (d == 0) return 0;
    uint64_t r = 0;
    for (int i = 63; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1);
        if (r >= d) {
            r -= d;
        }
    }
    return r;
}

static int64_t sdiv64(int64_t n, int64_t d) {
    if (d == 0) return 0;
    int sign = 1;
    if (n < 0) {
        sign = -sign;
        n = -n;
    }
    if (d < 0) {
        sign = -sign;
        d = -d;
    }
    uint64_t q = udiv64((uint64_t)n, (uint64_t)d);
    return sign < 0 ? -(int64_t)q : (int64_t)q;
}

/* Parse display string into fixed-point value scaled by CALC_SCALE */
static int64_t parse_display_value(const char *str) {
    if (!str || str[0] == '\0') return 0;

    int sign = 1;
    int i = 0;
    if (str[0] == '-') {
        sign = -1;
        i++;
    } else if (str[0] == '+') {
        i++;
    }

    int64_t int_part = 0;
    while (str[i] >= '0' && str[i] <= '9') {
        int_part = int_part * 10 + (str[i] - '0');
        i++;
    }

    int64_t frac_part = 0;
    if (str[i] == '.') {
        i++;
        int factor = 100;
        int digits = 0;
        while (str[i] >= '0' && str[i] <= '9' && digits < 3) {
            frac_part += (str[i] - '0') * factor;
            factor /= 10;
            digits++;
            i++;
        }
    }

    int64_t res = int_part * CALC_SCALE + frac_part;
    return sign < 0 ? -res : res;
}

/* Format fixed-point value into display string */
static void format_display_value(int64_t val, char *buf, size_t max_len) {
    if (max_len < 16) return;

    if (val == 0) {
        strcpy(buf, "0");
        return;
    }

    int sign = 0;
    if (val < 0) {
        sign = 1;
        val = -val;
    }

    uint64_t int_part = udiv64((uint64_t)val, CALC_SCALE);
    uint64_t frac_part = umod64((uint64_t)val, CALC_SCALE);

    char tmp[32];
    int idx = 0;

    if (int_part == 0) {
        tmp[idx++] = '0';
    } else {
        char rev[32];
        int r_idx = 0;
        while (int_part > 0) {
            rev[r_idx++] = (char)('0' + (int)umod64(int_part, 10));
            int_part = udiv64(int_part, 10);
        }
        for (int j = r_idx - 1; j >= 0; j--) {
            tmp[idx++] = rev[j];
        }
    }

    if (frac_part > 0) {
        tmp[idx++] = '.';
        uint32_t f = (uint32_t)frac_part;
        int d1 = (int)(f / 100);
        int d2 = (int)((f / 10) % 10);
        int d3 = (int)(f % 10);

        tmp[idx++] = (char)('0' + d1);
        if (d2 > 0 || d3 > 0) {
            tmp[idx++] = (char)('0' + d2);
            if (d3 > 0) {
                tmp[idx++] = (char)('0' + d3);
            }
        }
    }
    tmp[idx] = '\0';

    if (sign) {
        buf[0] = '-';
        strncpy(buf + 1, tmp, max_len - 2);
        buf[max_len - 1] = '\0';
    } else {
        strncpy(buf, tmp, max_len - 1);
        buf[max_len - 1] = '\0';
    }
}

/* Evaluate chained expression respecting operator precedence */
static void calc_evaluate(void) {
    if (g_calc.has_error || g_calc.token_count == 0) return;

    /* Append current input as final value if needed */
    if (!g_calc.new_number || g_calc.token_count == 0) {
        if (g_calc.token_count < MAX_TOKENS) {
            g_calc.values[g_calc.token_count] = parse_display_value(g_calc.current_input);
            g_calc.token_count++;
        }
    }

    if (g_calc.token_count == 0) return;

    int64_t vals[MAX_TOKENS];
    char ops[MAX_TOKENS];
    int count = g_calc.token_count;

    for (int i = 0; i < count; i++) {
        vals[i] = g_calc.values[i];
    }
    for (int i = 0; i < count - 1; i++) {
        ops[i] = g_calc.ops[i];
    }

    /* Pass 1: Multiplication and Division */
    for (int i = 0; i < count - 1; ) {
        if (ops[i] == '*' || ops[i] == '/') {
            if (ops[i] == '*') {
                vals[i] = sdiv64(vals[i] * vals[i + 1], CALC_SCALE);
            } else {
                if (vals[i + 1] == 0) {
                    g_calc.has_error = true;
                    strcpy(g_calc.current_input, "Error: Div by 0");
                    strcpy(g_calc.expr_history, "Math Error");
                    return;
                }
                vals[i] = sdiv64(vals[i] * CALC_SCALE, vals[i + 1]);
            }

            for (int j = i + 1; j < count - 1; j++) {
                vals[j] = vals[j + 1];
            }
            for (int j = i; j < count - 2; j++) {
                ops[j] = ops[j + 1];
            }
            count--;
        } else {
            i++;
        }
    }

    /* Pass 2: Addition and Subtraction */
    int64_t result = vals[0];
    for (int i = 0; i < count - 1; i++) {
        if (ops[i] == '+') {
            result += vals[i + 1];
        } else if (ops[i] == '-') {
            result -= vals[i + 1];
        }
    }

    format_display_value(result, g_calc.current_input, sizeof(g_calc.current_input));
    strcat(g_calc.expr_history, " = ");
    strcat(g_calc.expr_history, g_calc.current_input);

    g_calc.token_count = 0;
    g_calc.new_number = true;
}

static void calc_add_digit(char c) {
    if (g_calc.has_error || g_calc.new_number) {
        g_calc.current_input[0] = c;
        g_calc.current_input[1] = '\0';
        g_calc.new_number = false;
        g_calc.has_error = false;
        return;
    }

    size_t len = strlen(g_calc.current_input);
    if (len == 1 && g_calc.current_input[0] == '0' && c != '.') {
        g_calc.current_input[0] = c;
        return;
    }

    if (len < sizeof(g_calc.current_input) - 2) {
        g_calc.current_input[len] = c;
        g_calc.current_input[len + 1] = '\0';
    }
}

static void calc_add_dot(void) {
    if (g_calc.has_error || g_calc.new_number) {
        strcpy(g_calc.current_input, "0.");
        g_calc.new_number = false;
        g_calc.has_error = false;
        return;
    }

    if (!strchr(g_calc.current_input, '.')) {
        size_t len = strlen(g_calc.current_input);
        if (len < sizeof(g_calc.current_input) - 2) {
            g_calc.current_input[len] = '.';
            g_calc.current_input[len + 1] = '\0';
        }
    }
}

static void calc_toggle_sign(void) {
    if (g_calc.has_error) return;
    if (strcmp(g_calc.current_input, "0") == 0) return;

    if (g_calc.current_input[0] == '-') {
        /* Remove minus */
        size_t len = strlen(g_calc.current_input);
        for (size_t i = 0; i < len; i++) {
            g_calc.current_input[i] = g_calc.current_input[i + 1];
        }
    } else {
        /* Add minus */
        char tmp[32];
        tmp[0] = '-';
        strncpy(tmp + 1, g_calc.current_input, sizeof(tmp) - 2);
        tmp[sizeof(tmp) - 1] = '\0';
        strcpy(g_calc.current_input, tmp);
    }
}

static void calc_backspace(void) {
    if (g_calc.has_error || g_calc.new_number) return;
    size_t len = strlen(g_calc.current_input);
    if (len > 0) {
        g_calc.current_input[len - 1] = '\0';
        if (g_calc.current_input[0] == '\0' || strcmp(g_calc.current_input, "-") == 0) {
            strcpy(g_calc.current_input, "0");
        }
    }
}

static void calc_clear(void) {
    g_calc.token_count = 0;
    strcpy(g_calc.current_input, "0");
    g_calc.expr_history[0] = '\0';
    g_calc.new_number = true;
    g_calc.has_error = false;
}

static void calc_add_operator(char op) {
    if (g_calc.has_error) return;

    if (g_calc.new_number && g_calc.token_count > 0) {
        /* Replace last operator */
        g_calc.ops[g_calc.token_count - 1] = op;
        size_t hlen = strlen(g_calc.expr_history);
        if (hlen >= 2) {
            g_calc.expr_history[hlen - 2] = op;
        }
        return;
    }

    if (g_calc.token_count < MAX_TOKENS) {
        g_calc.values[g_calc.token_count] = parse_display_value(g_calc.current_input);
        g_calc.ops[g_calc.token_count] = op;
        g_calc.token_count++;

        if (g_calc.token_count == 1) {
            strncpy(g_calc.expr_history, g_calc.current_input, sizeof(g_calc.expr_history) - 8);
        } else {
            strncat(g_calc.expr_history, g_calc.current_input, 16);
        }

        char op_str[4];
        op_str[0] = ' ';
        op_str[1] = op;
        op_str[2] = ' ';
        op_str[3] = '\0';
        strncat(g_calc.expr_history, op_str, 4);

        g_calc.new_number = true;
    }
}

/* Button Layout Definition */
typedef struct {
    int col, row, col_span;
    const char *label;
    char key_id;
    color_t bg_color;
} calc_button_t;

static const calc_button_t g_calc_buttons[] = {
    { 0, 0, 1, "C",   'C', COLOR_ARGB(210, 190, 100, 140) },
    { 1, 0, 1, "+/-", 'S', COLOR_ARGB(200, 140, 100, 180) },
    { 2, 0, 1, "<-",  'B', COLOR_ARGB(200, 140, 100, 180) },
    { 3, 0, 1, "/",   '/', COLOR_ARGB(230, 255, 105, 180) },

    { 0, 1, 1, "7",   '7', COLOR_ARGB(190, 50, 35, 75) },
    { 1, 1, 1, "8",   '8', COLOR_ARGB(190, 50, 35, 75) },
    { 2, 1, 1, "9",   '9', COLOR_ARGB(190, 50, 35, 75) },
    { 3, 1, 1, "*",   '*', COLOR_ARGB(230, 255, 105, 180) },

    { 0, 2, 1, "4",   '4', COLOR_ARGB(190, 50, 35, 75) },
    { 1, 2, 1, "5",   '5', COLOR_ARGB(190, 50, 35, 75) },
    { 2, 2, 1, "6",   '6', COLOR_ARGB(190, 50, 35, 75) },
    { 3, 2, 1, "-",   '-', COLOR_ARGB(230, 255, 105, 180) },

    { 0, 3, 1, "1",   '1', COLOR_ARGB(190, 50, 35, 75) },
    { 1, 3, 1, "2",   '2', COLOR_ARGB(190, 50, 35, 75) },
    { 2, 3, 1, "3",   '3', COLOR_ARGB(190, 50, 35, 75) },
    { 3, 3, 1, "+",   '+', COLOR_ARGB(230, 255, 105, 180) },

    { 0, 4, 2, "0",   '0', COLOR_ARGB(190, 50, 35, 75) },
    { 2, 4, 1, ".",   '.', COLOR_ARGB(190, 50, 35, 75) },
    { 3, 4, 1, "=",   '=', COLOR_ARGB(240, 255, 60, 150) }
};
#define CALC_BUTTON_COUNT (sizeof(g_calc_buttons) / sizeof(g_calc_buttons[0]))

static void calc_execute_action(char action) {
    if (action >= '0' && action <= '9') {
        calc_add_digit(action);
    } else if (action == '.') {
        calc_add_dot();
    } else if (action == '+' || action == '-' || action == '*' || action == '/') {
        calc_add_operator(action);
    } else if (action == '=' || action == '\n' || action == '\r') {
        calc_evaluate();
    } else if (action == 'C' || action == 'c') {
        calc_clear();
    } else if (action == 'S') {
        calc_toggle_sign();
    } else if (action == 'B' || action == '\b') {
        calc_backspace();
    }
}

static void calc_draw(window_t *win) {
    int start_x = win->x;
    int start_y = win->y + WINDOW_TITLE_HEIGHT;

    int mx = mouse_get_x();
    int my = mouse_get_y();
    int lb = mouse_left_button();

    /* 1. Draw Display Box */
    int disp_x = start_x + 14;
    int disp_y = start_y + 12;
    int disp_w = win->width - 28;
    int disp_h = 60;

    gfx_fill_rounded_rect(disp_x, disp_y, disp_w, disp_h, 8, COLOR_ARGB(230, 24, 14, 42));
    gfx_draw_rounded_rect(disp_x, disp_y, disp_w, disp_h, 8, COLOR_ARGB(120, 255, 182, 193), 1);

    /* Chained Expression History */
    if (g_calc.expr_history[0] != '\0') {
        font_draw_string(disp_x + 10, disp_y + 8, g_calc.expr_history, STELLA_LAVENDER, 0);
    }

    /* Main input / result string */
    int text_w = font_string_width(g_calc.current_input);
    int num_x = disp_x + disp_w - text_w - 12;
    if (num_x < disp_x + 10) num_x = disp_x + 10;
    font_draw_string(num_x, disp_y + 34, g_calc.current_input, STELLA_TEXT_WHITE, 0);

    /* 2. Draw Buttons */
    int btn_start_x = start_x + 14;
    int btn_start_y = start_y + 84;
    int btn_base_w = 52;
    int btn_h = 42;
    int gap = 8;

    for (size_t i = 0; i < CALC_BUTTON_COUNT; i++) {
        const calc_button_t *b = &g_calc_buttons[i];
        int bx = btn_start_x + b->col * (btn_base_w + gap);
        int by = btn_start_y + b->row * (btn_h + gap);
        int bw = b->col_span == 2 ? (btn_base_w * 2 + gap) : btn_base_w;

        bool hover = (mx >= bx && mx < bx + bw && my >= by && my < by + btn_h);
        color_t fill = b->bg_color;

        if (hover && lb) {
            fill = STELLA_DEEP_PINK;
        } else if (hover) {
            fill = gfx_lerp_color(fill, COLOR_RGB(255, 200, 230), 80);
        }

        gfx_fill_rounded_rect(bx, by, bw, btn_h, 8, fill);
        gfx_draw_rounded_rect(bx, by, bw, btn_h, 8, COLOR_ARGB(90, 255, 255, 255), 1);

        int lbl_w = font_string_width(b->label);
        int lbl_x = bx + (bw - lbl_w) / 2;
        int lbl_y = by + (btn_h - 16) / 2;
        font_draw_string(lbl_x, lbl_y, b->label, STELLA_TEXT_WHITE, 0);
    }
}

static void calc_click(window_t *win, int cx, int cy) {
    UNUSED(win);
    int btn_start_x = 14;
    int btn_start_y = 84;
    int btn_base_w = 52;
    int btn_h = 42;
    int gap = 8;

    for (size_t i = 0; i < CALC_BUTTON_COUNT; i++) {
        const calc_button_t *b = &g_calc_buttons[i];
        int bx = btn_start_x + b->col * (btn_base_w + gap);
        int by = btn_start_y + b->row * (btn_h + gap);
        int bw = b->col_span == 2 ? (btn_base_w * 2 + gap) : btn_base_w;

        if (cx >= bx && cx < bx + bw && cy >= by && cy < by + btn_h) {
            calc_execute_action(b->key_id);
            break;
        }
    }
}

static void calc_key(window_t *win, char key) {
    UNUSED(win);
    calc_execute_action(key);
}

static void calc_close(window_t *win) {
    UNUSED(win);
    g_calc_win = NULL;
}

void app_calculator_open(void) {
    if (g_calc_win) {
        gui_focus_window(g_calc_win);
        return;
    }

    g_calc_win = gui_create_window("Calculator *", 180, 100, 260, 368);
    if (!g_calc_win) return;

    g_calc_win->on_draw  = calc_draw;
    g_calc_win->on_click = calc_click;
    g_calc_win->on_key   = calc_key;
    g_calc_win->on_close = calc_close;

    memset(&g_calc, 0, sizeof(calc_state_t));
    strcpy(g_calc.current_input, "0");
    g_calc.new_number = true;
}
