/*
 * StellaOS - sysmon.c
 * Kawaii System Monitor & Task Manager application
 * Copyright (c) 2024 StellaOS Project
 */

#include "sysmon.h"
#include "gui.h"
#include "framebuffer.h"
#include "graphics.h"
#include "font.h"
#include "memory.h"
#include "string.h"
#include "timer.h"
#include "mouse.h"

typedef struct {
    int pid;
    const char *name;
    const char *type;
    const char *status;
    int base_cpu;
    int memory_kb;
} sysmon_task_t;

static const sysmon_task_t g_tasks[] = {
    { 0, "kernel_idle",   "Kernel", "IDLE",    84, 4 },
    { 1, "compositor",    "System", "RUNNING",  8, 256 },
    { 2, "ps2_input",     "Driver", "SLEEP",    1, 16 },
    { 3, "pit_timer",     "Driver", "ACTIVE",   2, 8 },
    { 4, "stella_wm",     "GUI",    "ACTIVE",   3, 512 },
    { 5, "sysmon",        "App",    "RUNNING",  2, 64 }
};
#define TASK_COUNT (sizeof(g_tasks) / sizeof(g_tasks[0]))

static window_t *g_sysmon_win = NULL;
static int g_selected_task = 0;

static void sysmon_draw(window_t *win) {
    int start_x = win->x;
    int start_y = win->y + WINDOW_TITLE_HEIGHT;
    int win_w   = win->width;
    int mx      = mouse_get_x();
    int my      = mouse_get_y();

    uint32_t ticks = timer_get_ticks();
    uint32_t sec   = ticks / 100;
    uint32_t mins  = (sec / 60) % 60;
    uint32_t hrs   = (sec / 3600);
    uint32_t s_rem = sec % 60;

    /* 1. Header: OS & System Overview */
    int top_y = start_y + 12;
    font_draw_string(start_x + 16, top_y, "StellaOS v0.1.0 (Hoshizora) - x86 32-bit", STELLA_PINK, 0);

    char upt_str[48];
    char h_str[8], m_str[8], s_str[8];
    itoa((int)hrs, h_str, 10);
    itoa((int)mins, m_str, 10);
    itoa((int)s_rem, s_str, 10);

    strcpy(upt_str, "Uptime: ");
    strcat(upt_str, h_str);
    strcat(upt_str, "h ");
    strcat(upt_str, m_str);
    strcat(upt_str, "m ");
    strcat(upt_str, s_str);
    strcat(upt_str, "s");
    font_draw_string(start_x + win_w - font_string_width(upt_str) - 16, top_y, upt_str, STELLA_TEXT_LIGHT, 0);

    /* 2. Memory Usage Bar Graph */
    int mem_y = top_y + 26;
    font_draw_string(start_x + 16, mem_y, "Memory Usage (Kernel Heap):", STELLA_SKY_BLUE, 0);

    uint32_t mem_total = memory_get_total();
    uint32_t mem_used  = memory_get_used();
    uint32_t mem_free  = memory_get_free();

    if (mem_total == 0) {
        mem_total = 16 * 1024 * 1024;
        mem_used  = 4 * 1024 * 1024;
        mem_free  = mem_total - mem_used;
    }

    int bar_x = start_x + 16;
    int bar_y = mem_y + 18;
    int bar_w = win_w - 32;
    int bar_h = 20;

    /* Bar Background */
    gfx_fill_rounded_rect(bar_x, bar_y, bar_w, bar_h, 6, COLOR_ARGB(160, 24, 16, 40));
    gfx_draw_rounded_rect(bar_x, bar_y, bar_w, bar_h, 6, COLOR_ARGB(120, 255, 182, 193), 1);

    uint32_t total_kb = mem_total / 1024;
    uint32_t used_kb  = mem_used / 1024;
    int used_pixels = (total_kb > 0) ? (int)((used_kb * (uint32_t)bar_w) / total_kb) : 0;
    if (used_pixels > bar_w - 2) used_pixels = bar_w - 2;
    if (used_pixels < 4) used_pixels = 4;

    /* Segment 1: Kernel core allocations (Deep Pink) */
    int seg1_w = used_pixels / 2;
    if (seg1_w > 0) {
        gfx_fill_rounded_rect(bar_x + 1, bar_y + 1, seg1_w, bar_h - 2, 5, STELLA_DEEP_PINK);
    }

    /* Segment 2: GUI and Application heap (Lavender) */
    int seg2_w = used_pixels - seg1_w;
    if (seg2_w > 0) {
        gfx_fill_rect(bar_x + 1 + seg1_w, bar_y + 1, seg2_w, bar_h - 2, STELLA_LAVENDER);
    }

    /* Memory summary string */
    char mem_str[64];
    char used_mb[12], total_mb[12], free_mb[12];
    itoa((int)(mem_used / (1024 * 1024)), used_mb, 10);
    itoa((int)(mem_total / (1024 * 1024)), total_mb, 10);
    itoa((int)(mem_free / (1024 * 1024)), free_mb, 10);

    strcpy(mem_str, "Used: ");
    strcat(mem_str, used_mb);
    strcat(mem_str, " MB / ");
    strcat(mem_str, total_mb);
    strcat(mem_str, " MB | Free: ");
    strcat(mem_str, free_mb);
    strcat(mem_str, " MB");
    font_draw_string(bar_x + 8, bar_y + 24, mem_str, STELLA_TEXT_WHITE, 0);

    /* 3. Performance & Sparkle Section */
    int perf_y = bar_y + 48;
    font_draw_string(start_x + 16, perf_y, "CPU Activity & Performance", STELLA_DEEP_PINK, 0);

    /* Sparkle decorations */
    int sp1_x = start_x + 240;
    int sp1_y = perf_y + 7;
    int sp_phase = (int)(ticks % 30);
    color_t sp_c = (sp_phase < 15) ? STELLA_PINK : STELLA_LAVENDER;

    gfx_draw_line(sp1_x - 4, sp1_y, sp1_x + 4, sp1_y, sp_c);
    gfx_draw_line(sp1_x, sp1_y - 4, sp1_x, sp1_y + 4, sp_c);
    fb_putpixel(sp1_x, sp1_y, COLOR_RGB(255, 255, 255));

    int sp2_x = start_x + win_w - 24;
    int sp2_y = perf_y + 7;
    gfx_draw_line(sp2_x - 3, sp2_y, sp2_x + 3, sp2_y, STELLA_SAKURA);
    gfx_draw_line(sp2_x, sp2_y - 3, sp2_x, sp2_y + 3, STELLA_SAKURA);

    /* Dynamic CPU estimate */
    int dynamic_cpu = 6 + (int)(ticks % 7);
    if (mouse_left_button()) dynamic_cpu += 9;

    char cpu_str[32];
    char cpu_val[8];
    itoa(dynamic_cpu, cpu_val, 10);
    strcpy(cpu_str, "Load: ");
    strcat(cpu_str, cpu_val);
    strcat(cpu_str, "%");
    font_draw_string(start_x + win_w - 110, perf_y, cpu_str, STELLA_TEXT_WHITE, 0);

    /* CPU Mini Bar */
    int cbar_x = start_x + 16;
    int cbar_y = perf_y + 18;
    int cbar_w = win_w - 32;
    int cbar_h = 10;
    gfx_fill_rounded_rect(cbar_x, cbar_y, cbar_w, cbar_h, 4, COLOR_ARGB(140, 25, 15, 40));
    int cpu_fill_w = (cbar_w * dynamic_cpu) / 100;
    if (cpu_fill_w > 0) {
        gfx_fill_rounded_rect(cbar_x, cbar_y, cpu_fill_w, cbar_h, 4, COLOR_RGB(100, 255, 180));
    }
    gfx_draw_rounded_rect(cbar_x, cbar_y, cbar_w, cbar_h, 4, COLOR_ARGB(90, 200, 160, 255), 1);

    /* 4. Process / Task List Table */
    int tbl_y = cbar_y + 22;
    font_draw_string(start_x + 16, tbl_y, "Process & Kernel Threads:", STELLA_SKY_BLUE, 0);

    int head_y = tbl_y + 18;
    gfx_fill_rounded_rect(start_x + 16, head_y, win_w - 32, 22, 6, COLOR_ARGB(180, 45, 25, 65));
    gfx_draw_rounded_rect(start_x + 16, head_y, win_w - 32, 22, 6, COLOR_ARGB(80, 255, 182, 193), 1);

    font_draw_string(start_x + 26,  head_y + 4, "PID",    STELLA_TEXT_LIGHT, 0);
    font_draw_string(start_x + 70,  head_y + 4, "NAME",   STELLA_TEXT_LIGHT, 0);
    font_draw_string(start_x + 220, head_y + 4, "TYPE",   STELLA_TEXT_LIGHT, 0);
    font_draw_string(start_x + 300, head_y + 4, "STATUS", STELLA_TEXT_LIGHT, 0);
    font_draw_string(start_x + 390, head_y + 4, "CPU%",   STELLA_TEXT_LIGHT, 0);
    font_draw_string(start_x + 450, head_y + 4, "MEM",    STELLA_TEXT_LIGHT, 0);

    int row_h = 22;
    int list_start_y = head_y + 24;

    for (size_t i = 0; i < TASK_COUNT; i++) {
        const sysmon_task_t *t = &g_tasks[i];
        int ry = list_start_y + (int)i * row_h;

        bool hover = (mx >= start_x + 16 && mx < start_x + win_w - 16 &&
                      my >= ry && my < ry + row_h);
        bool selected = (g_selected_task == (int)i);

        if (selected) {
            gfx_fill_rounded_rect(start_x + 16, ry, win_w - 32, row_h - 2, 4, COLOR_ARGB(140, 255, 105, 180));
        } else if (hover) {
            gfx_fill_rounded_rect(start_x + 16, ry, win_w - 32, row_h - 2, 4, STELLA_HOVER);
        } else if (i % 2 == 1) {
            gfx_fill_rounded_rect(start_x + 16, ry, win_w - 32, row_h - 2, 4, COLOR_ARGB(70, 35, 20, 55));
        }

        char pid_s[8], cpu_s[8], mem_s[16];
        itoa(t->pid, pid_s, 10);
        itoa(t->base_cpu, cpu_s, 10);
        strcat(cpu_s, "%");
        itoa(t->memory_kb, mem_s, 10);
        strcat(mem_s, " KB");

        color_t stat_color = (strcmp(t->status, "RUNNING") == 0) ? COLOR_RGB(100, 255, 180) :
                             (strcmp(t->status, "ACTIVE") == 0)  ? STELLA_PINK :
                             (strcmp(t->status, "IDLE") == 0)    ? STELLA_SKY_BLUE : STELLA_TEXT_LIGHT;

        font_draw_string(start_x + 26,  ry + 3, pid_s,     STELLA_TEXT_WHITE, 0);
        font_draw_string(start_x + 70,  ry + 3, t->name,    STELLA_TEXT_WHITE, 0);
        font_draw_string(start_x + 220, ry + 3, t->type,    STELLA_LAVENDER, 0);
        font_draw_string(start_x + 300, ry + 3, t->status,  stat_color, 0);
        font_draw_string(start_x + 390, ry + 3, cpu_s,     STELLA_TEXT_WHITE, 0);
        font_draw_string(start_x + 450, ry + 3, mem_s,     STELLA_TEXT_LIGHT, 0);
    }
}

static void sysmon_click(window_t *win, int cx, int cy) {
    UNUSED(win);
    UNUSED(cx);
    int head_y = 12 + 26 + 18 + 20 + 28 + 18 + 10 + 22 + 18;
    int list_start_y = head_y + 24;
    int row_h = 22;

    for (size_t i = 0; i < TASK_COUNT; i++) {
        int ry = list_start_y + (int)i * row_h;
        if (cy >= ry && cy < ry + row_h) {
            g_selected_task = (int)i;
            break;
        }
    }
}

static void sysmon_close(window_t *win) {
    UNUSED(win);
    g_sysmon_win = NULL;
}

void app_sysmon_open(void) {
    if (g_sysmon_win) {
        gui_focus_window(g_sysmon_win);
        return;
    }

    g_sysmon_win = gui_create_window("System Monitor *", 140, 70, 540, 390);
    if (!g_sysmon_win) return;

    g_sysmon_win->on_draw  = sysmon_draw;
    g_sysmon_win->on_click = sysmon_click;
    g_sysmon_win->on_close = sysmon_close;
}
