/*
 * StellaOS - gui.h
 * Window manager and desktop environment interface
 * Copyright (c) 2024 StellaOS Project
 */

#ifndef STELLA_GUI_H
#define STELLA_GUI_H

#include "types.h"

typedef struct window_t window_t;

struct window_t {
    int x, y, width, height;
    char title[64];
    bool visible, focused, minimized, maximized;
    bool dragging;
    int drag_offset_x, drag_offset_y;
    color_t *buffer;  // window content buffer
    int id;
    void (*on_draw)(struct window_t *win);
    void (*on_key)(struct window_t *win, char key);
    void (*on_click)(struct window_t *win, int x, int y);
    void (*on_close)(struct window_t *win);

    /* Saved geometry for maximize / restore */
    int saved_x, saved_y, saved_width, saved_height;
};

void gui_init(void);
void gui_main_loop(void);  // main event loop
window_t *gui_create_window(const char *title, int x, int y, int w, int h);
void gui_close_window(window_t *win);
void gui_focus_window(window_t *win);
window_t *gui_get_focused_window(void);

#endif /* STELLA_GUI_H */
