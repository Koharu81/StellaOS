/*
 * StellaOS - wallpaper.h
 * Procedural wallpaper subsystem
 * Copyright (c) 2024 StellaOS Project
 */

#ifndef STELLA_WALLPAPER_H
#define STELLA_WALLPAPER_H

#include "types.h"

typedef struct {
    char name[32];
    void (*draw_func)(void);
} wallpaper_t;

void wallpaper_init(void);
void wallpaper_draw(void);
void wallpaper_next(void);
void wallpaper_prev(void);
void wallpaper_set(int index);
int wallpaper_get_count(void);
const char *wallpaper_get_name(int index);
int wallpaper_get_current(void);

#endif /* STELLA_WALLPAPER_H */
