/*
 * StellaOS - notification.h
 * Toast notification popup system
 * Copyright (c) 2024 StellaOS Project
 */

#ifndef STELLA_NOTIFICATION_H
#define STELLA_NOTIFICATION_H

#include "types.h"

#define NOTIFICATION_MAX 5

typedef enum {
    NOTIF_INFO = 0,
    NOTIF_SUCCESS,
    NOTIF_WARNING,
    NOTIF_ERROR
} notif_icon_t;

typedef struct {
    char title[48];
    char message[96];
    int icon_type;
    uint32_t duration_ms;
    uint32_t birth_tick;
    bool active;
} notification_t;

void notification_init(void);
void notification_show(const char *title, const char *message, int icon_type);
void notification_draw(void);
void notification_update(void);

#endif /* STELLA_NOTIFICATION_H */
