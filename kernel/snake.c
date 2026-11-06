/*
 * StellaOS - snake.c
 * Kawaii desktop snake game application
 * Copyright (c) 2024 StellaOS Project
 */

#include "snake.h"
#include "gui.h"
#include "graphics.h"
#include "font.h"
#include "framebuffer.h"
#include "memory.h"
#include "string.h"
#include "keyboard.h"
#include "timer.h"
#include "mouse.h"

#define GRID_COLS 20
#define GRID_ROWS 15
#define CELL_SIZE 18
#define MAX_SNAKE_LEN (GRID_COLS * GRID_ROWS)

typedef struct {
    int x, y;
} point_t;

typedef struct {
    point_t body[MAX_SNAKE_LEN];
    int length;
    int dir_x, dir_y;
    int next_dir_x, next_dir_y;
    point_t food;
    int score;
    int high_score;
    bool game_over;
    uint32_t last_move_tick;
    uint32_t move_delay;
    uint32_t rand_seed;
} snake_game_t;

static window_t *g_snake_win = NULL;
static snake_game_t g_snake;

static uint32_t snake_rand(void) {
    g_snake.rand_seed = g_snake.rand_seed * 1103515245 + 12345;
    return (g_snake.rand_seed >> 16) & 0x7FFF;
}

static void snake_spawn_food(void) {
    bool valid = false;
    int attempts = 0;

    while (!valid && attempts < 100) {
        g_snake.food.x = (int)(snake_rand() % GRID_COLS);
        g_snake.food.y = (int)(snake_rand() % GRID_ROWS);

        valid = true;
        for (int i = 0; i < g_snake.length; i++) {
            if (g_snake.body[i].x == g_snake.food.x && g_snake.body[i].y == g_snake.food.y) {
                valid = false;
                break;
            }
        }
        attempts++;
    }
}

static void snake_reset_game(void) {
    g_snake.length = 3;
    g_snake.body[0].x = 10;
    g_snake.body[0].y = 7;
    g_snake.body[1].x = 9;
    g_snake.body[1].y = 7;
    g_snake.body[2].x = 8;
    g_snake.body[2].y = 7;

    g_snake.dir_x = 1;
    g_snake.dir_y = 0;
    g_snake.next_dir_x = 1;
    g_snake.next_dir_y = 0;

    g_snake.score = 0;
    g_snake.game_over = false;
    g_snake.move_delay = 18; /* ~180ms initially */
    g_snake.last_move_tick = timer_get_ticks();

    snake_spawn_food();
}

static void snake_update(void) {
    if (g_snake.game_over) return;

    uint32_t ticks = timer_get_ticks();
    if (ticks - g_snake.last_move_tick < g_snake.move_delay) {
        return;
    }
    g_snake.last_move_tick = ticks;

    /* Apply buffered direction */
    g_snake.dir_x = g_snake.next_dir_x;
    g_snake.dir_y = g_snake.next_dir_y;

    int new_x = g_snake.body[0].x + g_snake.dir_x;
    int new_y = g_snake.body[0].y + g_snake.dir_y;

    /* Check wall collisions */
    if (new_x < 0 || new_x >= GRID_COLS || new_y < 0 || new_y >= GRID_ROWS) {
        g_snake.game_over = true;
        return;
    }

    /* Check self collision */
    for (int i = 0; i < g_snake.length - 1; i++) {
        if (g_snake.body[i].x == new_x && g_snake.body[i].y == new_y) {
            g_snake.game_over = true;
            return;
        }
    }

    /* Check food eating */
    bool ate_food = (new_x == g_snake.food.x && new_y == g_snake.food.y);

    if (ate_food) {
        if (g_snake.length < MAX_SNAKE_LEN) {
            g_snake.length++;
        }
        g_snake.score += 10;
        if (g_snake.score > g_snake.high_score) {
            g_snake.high_score = g_snake.score;
        }

        /* Speed up slightly */
        int speed_bonus = g_snake.score / 30;
        int delay = 18 - speed_bonus;
        g_snake.move_delay = (delay < 6) ? 6 : (uint32_t)delay;

        snake_spawn_food();
    }

    /* Move body */
    for (int i = g_snake.length - 1; i > 0; i--) {
        g_snake.body[i] = g_snake.body[i - 1];
    }
    g_snake.body[0].x = new_x;
    g_snake.body[0].y = new_y;
}

static void snake_draw(window_t *win) {
    /* Update game state each frame */
    snake_update();

    int start_x = win->x;
    int start_y = win->y + WINDOW_TITLE_HEIGHT;
    int win_w   = win->width;

    int mx = mouse_get_x();
    int my = mouse_get_y();

    /* 1. Header: Score & High Score */
    int header_y = start_y + 10;
    char score_str[48];
    char sc_val[8], hi_val[8];
    itoa(g_snake.score, sc_val, 10);
    itoa(g_snake.high_score, hi_val, 10);

    strcpy(score_str, "Score: ");
    strcat(score_str, sc_val);
    strcat(score_str, "   Best: ");
    strcat(score_str, hi_val);
    font_draw_string(start_x + 16, header_y, score_str, STELLA_PINK, 0);

    /* Decorative Sparkle icon */
    font_draw_string(start_x + win_w - 48, header_y, "[ * ]", STELLA_LAVENDER, 0);

    /* 2. Game Grid Board */
    int grid_x = start_x + 12;
    int grid_y = start_y + 36;
    int grid_w = GRID_COLS * CELL_SIZE;
    int grid_h = GRID_ROWS * CELL_SIZE;

    gfx_fill_rounded_rect(grid_x, grid_y, grid_w, grid_h, 6, COLOR_ARGB(230, 20, 12, 34));
    gfx_draw_rounded_rect(grid_x, grid_y, grid_w, grid_h, 6, COLOR_ARGB(90, 200, 160, 255), 1);

    /* Grid pattern dots */
    for (int r = 1; r < GRID_ROWS; r++) {
        for (int c = 1; c < GRID_COLS; c++) {
            fb_putpixel(grid_x + c * CELL_SIZE, grid_y + r * CELL_SIZE, COLOR_ARGB(40, 255, 255, 255));
        }
    }

    /* 3. Draw Food (Star / Sparkle) */
    int fx = grid_x + g_snake.food.x * CELL_SIZE + CELL_SIZE / 2;
    int fy = grid_y + g_snake.food.y * CELL_SIZE + CELL_SIZE / 2;
    color_t food_color = COLOR_RGB(255, 220, 90); /* Golden starlight */
    gfx_fill_circle(fx, fy, 3, food_color);
    gfx_draw_line(fx - 6, fy, fx + 6, fy, food_color);
    gfx_draw_line(fx, fy - 6, fx, fy + 6, food_color);
    fb_putpixel(fx, fy, COLOR_RGB(255, 255, 255));

    /* 4. Draw Snake Body & Head */
    for (int i = g_snake.length - 1; i >= 0; i--) {
        int seg_x = grid_x + g_snake.body[i].x * CELL_SIZE + 1;
        int seg_y = grid_y + g_snake.body[i].y * CELL_SIZE + 1;
        int seg_w = CELL_SIZE - 2;
        int seg_h = CELL_SIZE - 2;

        if (i == 0) {
            /* Head: Bright Deep Pink with cute eyes */
            gfx_fill_rounded_rect(seg_x, seg_y, seg_w, seg_h, 6, STELLA_DEEP_PINK);

            /* Cute eyes */
            int eye1_x = seg_x + 4, eye1_y = seg_y + 4;
            int eye2_x = seg_x + seg_w - 5, eye2_y = seg_y + 4;

            if (g_snake.dir_x > 0) {
                eye1_x = seg_x + seg_w - 4; eye1_y = seg_y + 3;
                eye2_x = seg_x + seg_w - 4; eye2_y = seg_y + seg_h - 4;
            } else if (g_snake.dir_x < 0) {
                eye1_x = seg_x + 3; eye1_y = seg_y + 3;
                eye2_x = seg_x + 3; eye2_y = seg_y + seg_h - 4;
            } else if (g_snake.dir_y > 0) {
                eye1_x = seg_x + 3; eye1_y = seg_y + seg_h - 4;
                eye2_x = seg_x + seg_w - 4; eye2_y = seg_y + seg_h - 4;
            }

            fb_putpixel(eye1_x, eye1_y, COLOR_RGB(255, 255, 255));
            fb_putpixel(eye2_x, eye2_y, COLOR_RGB(255, 255, 255));
        } else {
            /* Body: Gradient from Deep Pink to Lavender */
            uint8_t factor = (uint8_t)((i * 255) / g_snake.length);
            color_t body_color = gfx_lerp_color(STELLA_DEEP_PINK, STELLA_LAVENDER, factor);
            gfx_fill_rounded_rect(seg_x, seg_y, seg_w, seg_h, 4, body_color);
        }
    }

    /* 5. Game Over Overlay */
    if (g_snake.game_over) {
        /* Semi-transparent dark curtain */
        gfx_fill_rounded_rect(grid_x, grid_y, grid_w, grid_h, 6, COLOR_ARGB(190, 20, 10, 30));

        const char *go_text = "GAME OVER  (>_<)";
        int go_w = font_string_width(go_text);
        font_draw_string(grid_x + (grid_w - go_w) / 2, grid_y + 70, go_text, STELLA_DEEP_PINK, 0);

        char final_sc[32];
        strcpy(final_sc, "Final Score: ");
        strcat(final_sc, sc_val);
        int fsc_w = font_string_width(final_sc);
        font_draw_string(grid_x + (grid_w - fsc_w) / 2, grid_y + 105, final_sc, STELLA_TEXT_WHITE, 0);

        /* Try Again Button */
        int btn_w = 120;
        int btn_h = 32;
        int btn_x = grid_x + (grid_w - btn_w) / 2;
        int btn_y = grid_y + 150;

        bool hover = (mx >= btn_x && mx < btn_x + btn_w && my >= btn_y && my < btn_y + btn_h);
        color_t b_col = hover ? STELLA_DEEP_PINK : COLOR_ARGB(210, 200, 80, 140);

        gfx_fill_rounded_rect(btn_x, btn_y, btn_w, btn_h, 8, b_col);
        gfx_draw_rounded_rect(btn_x, btn_y, btn_w, btn_h, 8, COLOR_RGB(255, 255, 255), 1);

        const char *retry_txt = "Try Again *";
        int r_w = font_string_width(retry_txt);
        font_draw_string(btn_x + (btn_w - r_w) / 2, btn_y + 8, retry_txt, STELLA_TEXT_WHITE, 0);
    }
}

static void snake_key(window_t *win, char key) {
    UNUSED(win);
    uint8_t sc = keyboard_get_scancode();

    if (g_snake.game_over) {
        if (key == '\n' || key == '\r' || key == ' ' || key == 'r' || key == 'R') {
            snake_reset_game();
        }
        return;
    }

    if ((sc == KEY_UP || key == 'w' || key == 'W' || key == 'k' || key == '8') && g_snake.dir_y != 1) {
        g_snake.next_dir_x = 0;
        g_snake.next_dir_y = -1;
    } else if ((sc == KEY_DOWN || key == 's' || key == 'S' || key == 'j' || key == '2') && g_snake.dir_y != -1) {
        g_snake.next_dir_x = 0;
        g_snake.next_dir_y = 1;
    } else if ((sc == KEY_LEFT || key == 'a' || key == 'A' || key == 'h' || key == '4') && g_snake.dir_x != 1) {
        g_snake.next_dir_x = -1;
        g_snake.next_dir_y = 0;
    } else if ((sc == KEY_RIGHT || key == 'd' || key == 'D' || key == 'l' || key == '6') && g_snake.dir_x != -1) {
        g_snake.next_dir_x = 1;
        g_snake.next_dir_y = 0;
    }
}

static void snake_click(window_t *win, int cx, int cy) {
    UNUSED(win);
    if (!g_snake.game_over) return;

    int grid_w = GRID_COLS * CELL_SIZE;
    int btn_w = 120;
    int btn_h = 32;
    int btn_x = 12 + (grid_w - btn_w) / 2;
    int btn_y = 36 + 150;

    if (cx >= btn_x && cx < btn_x + btn_w && cy >= btn_y && cy < btn_y + btn_h) {
        snake_reset_game();
    }
}

static void snake_close(window_t *win) {
    UNUSED(win);
    g_snake_win = NULL;
}

void app_snake_open(void) {
    if (g_snake_win) {
        gui_focus_window(g_snake_win);
        return;
    }

    g_snake_win = gui_create_window("StellaSnake *", 150, 70, 384, 350);
    if (!g_snake_win) return;

    g_snake_win->on_draw  = snake_draw;
    g_snake_win->on_key   = snake_key;
    g_snake_win->on_click = snake_click;
    g_snake_win->on_close = snake_close;

    g_snake.rand_seed = timer_get_ticks() + 4242;
    snake_reset_game();
}
