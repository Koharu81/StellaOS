/*
 * StellaOS - crash.c
 * Kawaii anime panic / error crash screen
 * Copyright (c) 2024 StellaOS Project
 */

#include "crash.h"
#include "framebuffer.h"
#include "graphics.h"
#include "font.h"
#include "keyboard.h"
#include "timer.h"
#include "string.h"
#include "io.h"

static void hex_to_str(uint32_t val, char *buf) {
    const char hex_chars[] = "0123456789ABCDEF";
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 7; i >= 0; i--) {
        buf[2 + (7 - i)] = hex_chars[(val >> (i * 4)) & 0xF];
    }
    buf[10] = '\0';
}

static void draw_sad_face_art(int cx, int cy) {
    /* Draw kawaii sad eyes and crying tears */
    /* Left eye: (╥ */
    gfx_fill_rect(cx - 36, cy, 14, 2, STELLA_DEEP_PINK);
    gfx_fill_rect(cx - 33, cy + 2, 2, 10, STELLA_DEEP_PINK);
    gfx_fill_rect(cx - 25, cy + 2, 2, 10, STELLA_DEEP_PINK);

    /* Right eye: ╥) */
    gfx_fill_rect(cx + 22, cy, 14, 2, STELLA_DEEP_PINK);
    gfx_fill_rect(cx + 25, cy + 2, 2, 10, STELLA_DEEP_PINK);
    gfx_fill_rect(cx + 33, cy + 2, 2, 10, STELLA_DEEP_PINK);

    /* Cute sad mouth: _ */
    gfx_fill_rect(cx - 8, cy + 12, 16, 2, STELLA_PINK);

    /* Falling tear droplets */
    gfx_fill_circle(cx - 29, cy + 16, 2, STELLA_SKY_BLUE);
    gfx_fill_circle(cx - 29, cy + 22, 1, STELLA_SKY_BLUE);
    gfx_fill_circle(cx + 29, cy + 16, 2, STELLA_SKY_BLUE);
    gfx_fill_circle(cx + 29, cy + 22, 1, STELLA_SKY_BLUE);

    /* Text face representation */
    const char *face_str = "( ; _ ; )";
    int fw = font_string_width(face_str);
    font_draw_string(cx - fw / 2, cy + 30, face_str, STELLA_LAVENDER, 0);
}

static void render_crash_frame(const char *error, const char *details, uint32_t error_code, registers_t *regs) {
    uint32_t sw = fb_get_width();
    uint32_t sh = fb_get_height();
    if (sw == 0) sw = DEFAULT_SCREEN_WIDTH;
    if (sh == 0) sh = DEFAULT_SCREEN_HEIGHT;

    /* 1. Kawaii Panic Dark Violet Gradient */
    gfx_fill_gradient_v(0, 0, (int)sw, (int)sh, COLOR_RGB(55, 14, 44), COLOR_RGB(24, 8, 32));

    /* 2. Floating Star Decorations in Background */
    uint32_t seed = 441199;
    for (int i = 0; i < 48; i++) {
        seed = seed * 1103515245 + 12345;
        int sx = (int)(seed % sw);
        seed = seed * 1103515245 + 12345;
        int sy = (int)(seed % sh);

        color_t sc = (i % 2 == 0) ? COLOR_ARGB(60, 255, 182, 193) : COLOR_ARGB(40, 200, 160, 255);
        fb_putpixel(sx, sy, sc);
    }

    /* 3. Main Crash Dialog Card */
    int box_w = 680;
    int box_h = regs ? 470 : 380;
    int box_x = ((int)sw - box_w) / 2;
    int box_y = ((int)sh - box_h) / 2;

    gfx_draw_shadow(box_x, box_y, box_w, box_h, 16, 18);
    gfx_fill_rounded_rect(box_x, box_y, box_w, box_h, 16, COLOR_ARGB(245, 34, 16, 48));
    gfx_draw_rounded_rect(box_x, box_y, box_w, box_h, 16, STELLA_DEEP_PINK, 2);

    /* Decorative stars on box corners and edges */
    font_draw_string(box_x + 16,         box_y + 14, "*", STELLA_PINK, 0);
    font_draw_string(box_x + box_w - 24, box_y + 14, "*", STELLA_PINK, 0);
    font_draw_string(box_x + 16,         box_y + box_h - 26, "*", STELLA_LAVENDER, 0);
    font_draw_string(box_x + box_w - 24, box_y + box_h - 26, "*", STELLA_LAVENDER, 0);

    /* Decorative star string across top */
    const char *stars_top = "*   ~   *   ~   *   ~   *";
    int stw = font_string_width(stars_top);
    font_draw_string(box_x + (box_w - stw) / 2, box_y + 14, stars_top, STELLA_SAKURA, 0);

    /* 4. Kawaii Sad Face Art */
    int cx = box_x + box_w / 2;
    draw_sad_face_art(cx, box_y + 44);

    /* 5. Error Heading */
    const char *heading = "Gomen ne! StellaOS encountered a problem...";
    int hw = font_string_width(heading);
    font_draw_string(cx - hw / 2, box_y + 104, heading, STELLA_DEEP_PINK, 0);

    const char *sub = "The system halted to protect your starlight data from corruption.";
    int sub_w = font_string_width(sub);
    font_draw_string(cx - sub_w / 2, box_y + 124, sub, STELLA_TEXT_LIGHT, 0);

    /* 6. Error Details Box */
    int det_y = box_y + 154;
    gfx_fill_rounded_rect(box_x + 28, det_y, box_w - 56, 76, 8, COLOR_ARGB(180, 20, 10, 32));
    gfx_draw_rounded_rect(box_x + 28, det_y, box_w - 56, 76, 8, COLOR_ARGB(100, 255, 105, 180), 1);

    char err_line[128];
    strcpy(err_line, "Fault: ");
    strncat(err_line, error ? error : "Kernel Exception", 64);
    font_draw_string(box_x + 40, det_y + 10, err_line, STELLA_TEXT_WHITE, 0);

    if (details) {
        char det_line[128];
        strcpy(det_line, "Details: ");
        strncat(det_line, details, 64);
        font_draw_string(box_x + 40, det_y + 30, det_line, STELLA_LAVENDER, 0);
    }

    char code_line[64];
    char hex_code[16];
    hex_to_str(error_code, hex_code);
    strcpy(code_line, "Error Code: ");
    strcat(code_line, hex_code);
    font_draw_string(box_x + 40, det_y + 50, code_line, COLOR_RGB(100, 255, 180), 0);

    /* 7. Register Dump (if provided) */
    if (regs) {
        int reg_y = det_y + 90;
        font_draw_string(box_x + 28, reg_y, "CPU Register State:", STELLA_SKY_BLUE, 0);

        int rtbl_y = reg_y + 18;
        gfx_fill_rounded_rect(box_x + 28, rtbl_y, box_w - 56, 94, 6, COLOR_ARGB(140, 20, 10, 32));
        gfx_draw_rounded_rect(box_x + 28, rtbl_y, box_w - 56, 94, 6, COLOR_ARGB(80, 200, 160, 255), 1);

        char buf1[32], buf2[32], buf3[32], buf4[32];
        char h[16];

        /* Row 1: EAX, EBX, ECX, EDX */
        hex_to_str(regs->eax, h); strcpy(buf1, "EAX:"); strcat(buf1, h);
        hex_to_str(regs->ebx, h); strcpy(buf2, "EBX:"); strcat(buf2, h);
        hex_to_str(regs->ecx, h); strcpy(buf3, "ECX:"); strcat(buf3, h);
        hex_to_str(regs->edx, h); strcpy(buf4, "EDX:"); strcat(buf4, h);
        font_draw_string(box_x + 40,  rtbl_y + 8, buf1, STELLA_TEXT_WHITE, 0);
        font_draw_string(box_x + 190, rtbl_y + 8, buf2, STELLA_TEXT_WHITE, 0);
        font_draw_string(box_x + 340, rtbl_y + 8, buf3, STELLA_TEXT_WHITE, 0);
        font_draw_string(box_x + 490, rtbl_y + 8, buf4, STELLA_TEXT_WHITE, 0);

        /* Row 2: ESI, EDI, EBP, ESP */
        hex_to_str(regs->esi, h); strcpy(buf1, "ESI:"); strcat(buf1, h);
        hex_to_str(regs->edi, h); strcpy(buf2, "EDI:"); strcat(buf2, h);
        hex_to_str(regs->ebp, h); strcpy(buf3, "EBP:"); strcat(buf3, h);
        hex_to_str(regs->esp, h); strcpy(buf4, "ESP:"); strcat(buf4, h);
        font_draw_string(box_x + 40,  rtbl_y + 28, buf1, STELLA_TEXT_WHITE, 0);
        font_draw_string(box_x + 190, rtbl_y + 28, buf2, STELLA_TEXT_WHITE, 0);
        font_draw_string(box_x + 340, rtbl_y + 28, buf3, STELLA_TEXT_WHITE, 0);
        font_draw_string(box_x + 490, rtbl_y + 28, buf4, STELLA_TEXT_WHITE, 0);

        /* Row 3: EIP, EFLAGS, CS, DS */
        hex_to_str(regs->eip, h);    strcpy(buf1, "EIP:"); strcat(buf1, h);
        hex_to_str(regs->eflags, h); strcpy(buf2, "FLG:"); strcat(buf2, h);
        hex_to_str(regs->cs, h);     strcpy(buf3, "CS :"); strcat(buf3, h);
        hex_to_str(regs->ds, h);     strcpy(buf4, "DS :"); strcat(buf4, h);
        font_draw_string(box_x + 40,  rtbl_y + 48, buf1, STELLA_TEXT_WHITE, 0);
        font_draw_string(box_x + 190, rtbl_y + 48, buf2, STELLA_TEXT_WHITE, 0);
        font_draw_string(box_x + 340, rtbl_y + 48, buf3, STELLA_TEXT_WHITE, 0);
        font_draw_string(box_x + 490, rtbl_y + 48, buf4, STELLA_TEXT_WHITE, 0);

        /* Row 4: INT, ERR */
        hex_to_str(regs->int_no, h);   strcpy(buf1, "INT:"); strcat(buf1, h);
        hex_to_str(regs->err_code, h); strcpy(buf2, "ERR:"); strcat(buf2, h);
        font_draw_string(box_x + 40,  rtbl_y + 68, buf1, STELLA_PINK, 0);
        font_draw_string(box_x + 190, rtbl_y + 68, buf2, STELLA_PINK, 0);
    }

    /* 8. Footer prompt */
    int foot_y = box_y + box_h - 38;
    const char *prompt = "Press any key to restart StellaOS ~";
    int pw = font_string_width(prompt);
    font_draw_string(cx - pw / 2, foot_y, prompt, STELLA_PINK, 0);

    fb_swap();
}

static void system_restart(void) {
    /* 1. Try 8042 Keyboard Controller pulse reset */
    uint8_t good = 0x02;
    while (good & 0x02) {
        good = inb(0x64);
    }
    outb(0x64, 0xFE);

    /* 2. Fallback: Triple fault via invalid IDT */
    struct {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed)) null_idtr = { 0, 0 };

    __asm__ volatile ("lidt %0; int3" : : "m"(null_idtr));

    /* 3. Infinite halt if CPU doesn't reset */
    while (1) {
        __asm__ volatile ("cli; hlt");
    }
}

void crash_screen(const char *error, const char *details, uint32_t error_code) {
    render_crash_frame(error, details, error_code, NULL);

    /* Wait for any keystroke */
    while (1) {
        if (keyboard_has_key()) {
            keyboard_getchar();
            system_restart();
        }
        timer_wait(20);
    }
}

void crash_screen_regs(const char *error, registers_t *regs) {
    uint32_t code = regs ? regs->err_code : 0;
    render_crash_frame(error, "CPU Exception / Interrupt", code, regs);

    while (1) {
        if (keyboard_has_key()) {
            keyboard_getchar();
            system_restart();
        }
        timer_wait(20);
    }
}
