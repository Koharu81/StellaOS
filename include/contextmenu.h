/*
 * StellaOS - contextmenu.h
 * Right-click context menu subsystem
 * Copyright (c) 2024 StellaOS Project
 */

#ifndef STELLA_CONTEXTMENU_H
#define STELLA_CONTEXTMENU_H

#include "types.h"

#define CONTEXTMENU_MAX_ITEMS 12

typedef struct {
    char label[32];
    void (*callback)(void);
    bool enabled;
    bool is_separator;
} context_menu_item_t;

void contextmenu_init(void);
void contextmenu_show(int x, int y, const char **items, int count, void (**callbacks)(void));
void contextmenu_hide(void);
bool contextmenu_is_visible(void);
bool contextmenu_handle_click(int mx, int my);
void contextmenu_draw(void);

#endif /* STELLA_CONTEXTMENU_H */
