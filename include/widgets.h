/*
 * StellaOS - widgets.h
 * Desktop widgets system
 * Copyright (c) 2024 StellaOS Project
 */

#ifndef STELLA_WIDGETS_H
#define STELLA_WIDGETS_H

#include "types.h"

typedef struct {
    int x, y;
    int width, height;
    void (*draw)(int x, int y, int w, int h);
} widget_t;

void widgets_init(void);
void widgets_draw(void);
void widgets_toggle(void);
bool widgets_visible(void);

#endif /* STELLA_WIDGETS_H */
