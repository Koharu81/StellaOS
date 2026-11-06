/*
 * StellaOS - types.h
 * Core type definitions for the kernel
 * Copyright (c) 2024 StellaOS Project - Open Source
 */

#ifndef STELLA_TYPES_H
#define STELLA_TYPES_H

/* Fixed-width integer types */
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef signed long long    int64_t;

typedef uint32_t            size_t;
typedef int32_t             ssize_t;
typedef uint32_t            uintptr_t;
typedef int32_t             intptr_t;

/* Boolean type */
typedef int                 bool;
#define true  1
#define false 0

/* NULL */
#define NULL ((void*)0)

/* Useful macros */
#define UNUSED(x)       ((void)(x))
#define ALIGN_UP(x, a)  (((x) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(x,a) ((x) & ~((a) - 1))
#define MIN(a, b)       ((a) < (b) ? (a) : (b))
#define MAX(a, b)       ((a) > (b) ? (a) : (b))
#define ABS(x)          ((x) < 0 ? -(x) : (x))
#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))

/* Color type (ARGB8888) */
typedef uint32_t color_t;

/* Color construction macros */
#define COLOR_ARGB(a, r, g, b)  (((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))
#define COLOR_RGB(r, g, b)      COLOR_ARGB(0xFF, r, g, b)
#define COLOR_A(c)              (((c) >> 24) & 0xFF)
#define COLOR_R(c)              (((c) >> 16) & 0xFF)
#define COLOR_G(c)              (((c) >>  8) & 0xFF)
#define COLOR_B(c)              ( (c)        & 0xFF)

/* Stella Theme Colors - Anime/Kawaii palette */
#define STELLA_PINK             COLOR_RGB(255, 182, 193)
#define STELLA_DEEP_PINK        COLOR_RGB(255, 105, 180)
#define STELLA_SAKURA           COLOR_RGB(255, 183, 197)
#define STELLA_LAVENDER         COLOR_RGB(230, 190, 255)
#define STELLA_LIGHT_PURPLE     COLOR_RGB(200, 160, 255)
#define STELLA_SKY_BLUE         COLOR_RGB(135, 206, 250)
#define STELLA_SOFT_WHITE       COLOR_RGB(250, 245, 255)
#define STELLA_DARK_BG          COLOR_ARGB(220, 30, 20, 50)
#define STELLA_PANEL_BG         COLOR_ARGB(200, 40, 30, 60)
#define STELLA_TASKBAR_BG       COLOR_ARGB(210, 25, 15, 45)
#define STELLA_TEXT_WHITE        COLOR_RGB(255, 255, 255)
#define STELLA_TEXT_LIGHT        COLOR_RGB(230, 220, 240)
#define STELLA_ACCENT            COLOR_RGB(255, 130, 200)
#define STELLA_HOVER             COLOR_ARGB(180, 255, 150, 210)
#define STELLA_WINDOW_BG         COLOR_ARGB(230, 35, 25, 55)
#define STELLA_WINDOW_TITLE_BG   COLOR_ARGB(240, 80, 40, 120)
#define STELLA_CLOSE_BTN         COLOR_RGB(255, 100, 100)
#define STELLA_MINIMIZE_BTN      COLOR_RGB(255, 200, 100)
#define STELLA_MAXIMIZE_BTN      COLOR_RGB(100, 255, 150)
#define STELLA_TRANSPARENT       COLOR_ARGB(0, 0, 0, 0)
#define STELLA_SHADOW            COLOR_ARGB(80, 0, 0, 0)
#define STELLA_SELECTION         COLOR_ARGB(150, 255, 130, 200)
#define STELLA_INSTALLER_BG      COLOR_ARGB(240, 20, 10, 40)

/* Screen dimensions (default, can be overridden by multiboot) */
#define DEFAULT_SCREEN_WIDTH    1280
#define DEFAULT_SCREEN_HEIGHT   720
#define DEFAULT_SCREEN_BPP     32

/* Keyboard scancodes (basic set) */
#define KEY_ESC         0x01
#define KEY_ENTER       0x1C
#define KEY_BACKSPACE   0x0E
#define KEY_TAB         0x0F
#define KEY_LSHIFT      0x2A
#define KEY_RSHIFT      0x36
#define KEY_LCTRL       0x1D
#define KEY_LALT        0x38
#define KEY_SPACE       0x39
#define KEY_CAPS        0x3A
#define KEY_F1          0x3B
#define KEY_F2          0x3C
#define KEY_F3          0x3D
#define KEY_F4          0x3E
#define KEY_UP          0x48
#define KEY_DOWN        0x50
#define KEY_LEFT        0x4B
#define KEY_RIGHT       0x4D

/* Window manager constants */
#define MAX_WINDOWS         32
#define WINDOW_TITLE_HEIGHT 32
#define WINDOW_BORDER_RADIUS 12
#define TASKBAR_HEIGHT      52
#define TASKBAR_MARGIN      8
#define TASKBAR_RADIUS      16
#define ICON_SIZE           32
#define CURSOR_SIZE         16

/* Memory constants */
#define PAGE_SIZE           4096
#define KERNEL_HEAP_START   0x00400000  /* 4MB */
#define KERNEL_HEAP_SIZE    0x01000000  /* 16MB */
#define KERNEL_HEAP_END     (KERNEL_HEAP_START + KERNEL_HEAP_SIZE)

/* Max string lengths */
#define MAX_PATH            256
#define MAX_FILENAME        64
#define MAX_LANG_NAME       32
#define MAX_LANG_COUNT      40

#endif /* STELLA_TYPES_H */

