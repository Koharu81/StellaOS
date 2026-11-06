/*
 * StellaOS - wallpaper.c
 * Procedural anime and aesthetic wallpapers for StellaOS
 * Copyright (c) 2024 StellaOS Project
 */

#include "wallpaper.h"
#include "framebuffer.h"
#include "graphics.h"
#include "timer.h"
#include "string.h"

#define WALLPAPER_COUNT 8
#define SAKURA_PETAL_COUNT 48
#define STAR_COUNT 72

/* Integer sine/cosine approximation (-127 to 127, angle 0..31 = 0..360 deg) */
static const int8_t g_sin_table[32] = {
    0,  24,  48,  71,  90, 106, 117, 125,
    127, 125, 117, 106,  90,  71,  48,  24,
    0, -24, -48, -71, -90,-106,-117,-125,
   -127,-125,-117,-106, -90, -71, -48, -24
};

static int isin(int angle) {
    int idx = (angle % 32 + 32) % 32;
    return g_sin_table[idx];
}

static int icos(int angle) {
    return isin(angle + 8);
}

typedef struct {
    int base_x, base_y;
    int speed_x, speed_y;
    int phase;
    uint8_t size;
} petal_t;

typedef struct {
    int x, y;
    uint8_t size;
    uint8_t phase;
    color_t color;
} star_node_t;

static petal_t g_petals[SAKURA_PETAL_COUNT];
static star_node_t g_wp_stars[STAR_COUNT];
static bool g_particles_init = false;
static int g_current_wallpaper = 0;

static void init_particles(uint32_t sw, uint32_t sh) {
    uint32_t seed = 771239;

    for (int i = 0; i < SAKURA_PETAL_COUNT; i++) {
        seed = seed * 1103515245 + 12345;
        g_petals[i].base_x = (int)(seed % sw);
        seed = seed * 1103515245 + 12345;
        g_petals[i].base_y = (int)(seed % sh);
        seed = seed * 1103515245 + 12345;
        g_petals[i].speed_x = (int)((seed % 3) + 1);
        seed = seed * 1103515245 + 12345;
        g_petals[i].speed_y = (int)((seed % 2) + 1);
        seed = seed * 1103515245 + 12345;
        g_petals[i].phase = (int)(seed % 32);
        seed = seed * 1103515245 + 12345;
        g_petals[i].size = (uint8_t)((seed % 3) + 2);
    }

    for (int i = 0; i < STAR_COUNT; i++) {
        seed = seed * 1103515245 + 12345;
        g_wp_stars[i].x = (int)(seed % sw);
        seed = seed * 1103515245 + 12345;
        g_wp_stars[i].y = (int)(seed % (sh - 40));
        seed = seed * 1103515245 + 12345;
        g_wp_stars[i].size = (uint8_t)((seed % 3) + 1);
        seed = seed * 1103515245 + 12345;
        g_wp_stars[i].phase = (uint8_t)(seed % 32);

        int c = (int)((seed >> 8) % 4);
        if (c == 0) g_wp_stars[i].color = STELLA_PINK;
        else if (c == 1) g_wp_stars[i].color = STELLA_LAVENDER;
        else if (c == 2) g_wp_stars[i].color = STELLA_SKY_BLUE;
        else g_wp_stars[i].color = COLOR_RGB(255, 255, 255);
    }

    g_particles_init = true;
}

/* 1. Starry Night */
static void draw_wp_starry_night(void) {
    uint32_t sw = fb_get_width();
    uint32_t sh = fb_get_height();
    uint32_t ticks = timer_get_ticks();

    gfx_fill_gradient_v(0, 0, (int)sw, (int)sh, COLOR_RGB(16, 9, 32), COLOR_RGB(46, 20, 62));

    for (int i = 0; i < STAR_COUNT; i++) {
        star_node_t *st = &g_wp_stars[i];
        int anim = (int)((ticks / 8 + st->phase) % 32);
        uint8_t alpha = (uint8_t)(130 + (anim > 16 ? (32 - anim) * 7 : anim * 7));
        color_t col = COLOR_ARGB(alpha, COLOR_R(st->color), COLOR_G(st->color), COLOR_B(st->color));

        if (st->size == 1) {
            fb_putpixel(st->x, st->y, col);
        } else if (st->size == 2) {
            gfx_fill_circle(st->x, st->y, 1, col);
        } else {
            gfx_fill_circle(st->x, st->y, 1, col);
            gfx_draw_line(st->x - 3, st->y, st->x + 3, st->y, col);
            gfx_draw_line(st->x, st->y - 3, st->x, st->y + 3, col);
            fb_putpixel(st->x, st->y, COLOR_RGB(255, 255, 255));
        }
    }
}

/* 2. Sakura Dream */
static void draw_wp_sakura_dream(void) {
    uint32_t sw = fb_get_width();
    uint32_t sh = fb_get_height();
    uint32_t ticks = timer_get_ticks();

    /* Dreamy twilight rose gradient */
    gfx_fill_gradient_v(0, 0, (int)sw, (int)sh, COLOR_RGB(55, 22, 45), COLOR_RGB(30, 14, 38));

    /* Distant soft moon glow */
    gfx_fill_circle((int)sw - 160, 120, 48, COLOR_ARGB(40, 255, 200, 220));
    gfx_fill_circle((int)sw - 160, 120, 36, COLOR_ARGB(90, 255, 220, 235));
    gfx_fill_circle((int)sw - 160, 120, 26, COLOR_RGB(255, 240, 248));

    /* Drifting Sakura Petals */
    for (int i = 0; i < SAKURA_PETAL_COUNT; i++) {
        petal_t *p = &g_petals[i];
        int t = (int)(ticks / 2);
        int sway = (isin(t / 4 + p->phase) * 16) / 127;

        int px = (p->base_x + t * p->speed_x + sway) % (int)sw;
        int py = (p->base_y + t * p->speed_y) % (int)sh;

        color_t pcol = (i % 2 == 0) ? STELLA_SAKURA : STELLA_DEEP_PINK;
        gfx_fill_circle(px, py, p->size, pcol);
        gfx_fill_circle(px + 2, py + 1, p->size - 1, COLOR_ARGB(180, 255, 230, 240));
    }
}

/* 3. Ocean Twilight */
static void draw_wp_ocean_twilight(void) {
    uint32_t sw = fb_get_width();
    uint32_t sh = fb_get_height();
    uint32_t ticks = timer_get_ticks();

    /* Deep Indigo sky down to twilight cyan */
    gfx_fill_gradient_v(0, 0, (int)sw, (int)sh, COLOR_RGB(10, 12, 34), COLOR_RGB(18, 48, 82));

    /* Stars in upper sky */
    for (int i = 0; i < 30; i++) {
        star_node_t *st = &g_wp_stars[i];
        if (st->y < (int)sh / 2) {
            fb_putpixel(st->x, st->y, st->color);
        }
    }

    /* Crescent Moon */
    int mx = 180, my = 100;
    gfx_fill_circle(mx, my, 28, COLOR_RGB(255, 250, 210));
    gfx_fill_circle(mx + 10, my - 6, 24, COLOR_RGB(10, 12, 34));

    /* Multi-layered animated wave bands */
    int wave_base_y = (int)sh - 180;
    int t = (int)(ticks / 4);

    for (int band = 0; band < 4; band++) {
        int by = wave_base_y + band * 40;
        color_t wave_col = gfx_lerp_color(COLOR_RGB(16, 42, 78), COLOR_RGB(25, 75, 120), (uint8_t)(band * 70));

        for (int x = 0; x < (int)sw; x += 4) {
            int wave_h = (isin((x / 16) + t + band * 8) * 8) / 127;
            gfx_fill_rect(x, by + wave_h, 4, (int)sh - (by + wave_h), wave_col);
            if (band > 0) {
                fb_putpixel(x, by + wave_h, COLOR_ARGB(140, 180, 230, 255));
            }
        }
    }
}

/* 4. Aurora */
static void draw_wp_aurora(void) {
    uint32_t sw = fb_get_width();
    uint32_t sh = fb_get_height();
    uint32_t ticks = timer_get_ticks();

    /* Arctic night sky */
    gfx_fill_gradient_v(0, 0, (int)sw, (int)sh, COLOR_RGB(8, 10, 24), COLOR_RGB(16, 18, 36));

    /* Distant stars */
    for (int i = 0; i < 40; i++) {
        star_node_t *st = &g_wp_stars[i];
        fb_putpixel(st->x, st->y, st->color);
    }

    /* Wavy animated aurora curtains */
    int t = (int)(ticks / 5);

    for (int x = 0; x < (int)sw; x += 3) {
        int y1 = (int)sh / 3 + (isin((x / 20) + t) * 35) / 127;
        int y2 = y1 + 55 + (icos((x / 18) - t) * 20) / 127;

        color_t aurora_green = COLOR_ARGB(50, 60, 245, 160);
        color_t aurora_cyan  = COLOR_ARGB(60, 0, 210, 255);

        gfx_fill_rect(x, y1, 3, y2 - y1, aurora_green);
        gfx_fill_rect(x, y1 + 15, 3, (y2 - y1) / 2, aurora_cyan);
    }
}

/* 5. Neon City */
static void draw_wp_neon_city(void) {
    uint32_t sw = fb_get_width();
    uint32_t sh = fb_get_height();
    uint32_t ticks = timer_get_ticks();

    /* Synthwave dark violet backdrop */
    gfx_fill_gradient_v(0, 0, (int)sw, (int)sh / 2, COLOR_RGB(14, 8, 25), COLOR_RGB(36, 14, 48));

    /* Giant glowing synthwave sun on the horizon */
    int sun_cx = (int)sw / 2;
    int horizon_y = (int)sh / 2;
    gfx_fill_circle(sun_cx, horizon_y, 75, COLOR_RGB(255, 60, 140));
    gfx_fill_circle(sun_cx, horizon_y, 60, COLOR_RGB(255, 120, 60));
    gfx_fill_circle(sun_cx, horizon_y, 45, COLOR_RGB(255, 210, 80));

    /* Sun horizontal scanlines */
    for (int ly = horizon_y - 60; ly < horizon_y; ly += 8) {
        gfx_fill_rect(sun_cx - 75, ly, 150, 2, COLOR_RGB(14, 8, 25));
    }

    /* Lower ground plane */
    gfx_fill_rect(0, horizon_y, (int)sw, (int)sh - horizon_y, COLOR_RGB(20, 10, 32));

    /* Perspective neon grid lines */
    int t = (int)((ticks / 2) % 30);

    /* Horizontal moving grid lines */
    for (int y = horizon_y; y < (int)sh; y += 12) {
        int py = y + (t * (y - horizon_y)) / 100;
        if (py < (int)sh) {
            gfx_draw_line(0, py, (int)sw, py, COLOR_ARGB(120, 255, 80, 180));
        }
    }

    /* Converging vertical lines towards sun center */
    for (int x = -100; x <= (int)sw + 100; x += 60) {
        gfx_draw_line(sun_cx, horizon_y, x, (int)sh, COLOR_ARGB(100, 160, 80, 240));
    }
}

/* 6. Galaxy */
static void draw_wp_galaxy(void) {
    uint32_t sw = fb_get_width();
    uint32_t sh = fb_get_height();
    uint32_t ticks = timer_get_ticks();

    /* Deep cosmic background */
    gfx_fill_gradient_v(0, 0, (int)sw, (int)sh, COLOR_RGB(6, 4, 16), COLOR_RGB(22, 10, 38));

    int cx = (int)sw / 2;
    int cy = (int)sh / 2 - 20;

    /* Galactic core glow rings */
    gfx_fill_circle(cx, cy, 50, COLOR_ARGB(35, 255, 180, 220));
    gfx_fill_circle(cx, cy, 32, COLOR_ARGB(70, 255, 120, 190));
    gfx_fill_circle(cx, cy, 18, COLOR_RGB(255, 240, 250));

    /* Dual spiral arms of stars */
    int t_angle = (int)(ticks / 6);

    for (int arm = 0; arm < 2; arm++) {
        int arm_offset = arm * 16;
        for (int r = 15; r < 240; r += 6) {
            int angle = (t_angle + arm_offset + r / 7) % 32;
            int sx = cx + (r * icos(angle)) / 127;
            int sy = cy + (r * isin(angle) * 7) / (127 * 10); /* Elliptical tilt */

            color_t star_c = (r % 12 == 0) ? STELLA_DEEP_PINK :
                             (r % 8 == 0)  ? STELLA_SKY_BLUE  : STELLA_LAVENDER;

            if (sx >= 0 && sx < (int)sw && sy >= 0 && sy < (int)sh) {
                fb_putpixel(sx, sy, star_c);
                if (r < 100) {
                    fb_putpixel(sx + 1, sy, star_c);
                }
            }
        }
    }
}

/* 7. Sunset */
static void draw_wp_sunset(void) {
    uint32_t sw = fb_get_width();
    uint32_t sh = fb_get_height();

    int mid_y = (int)sh / 2;

    /* Warm Anime Twilight Sunset Gradient */
    gfx_fill_gradient_v(0, 0, (int)sw, mid_y, COLOR_RGB(42, 18, 56), COLOR_RGB(215, 65, 85));
    gfx_fill_gradient_v(0, mid_y, (int)sw, (int)sh - mid_y, COLOR_RGB(215, 65, 85), COLOR_RGB(255, 175, 75));

    /* Giant Setting Sun */
    int sun_cx = (int)sw - 260;
    int sun_cy = mid_y + 20;
    gfx_fill_circle(sun_cx, sun_cy, 68, COLOR_ARGB(60, 255, 220, 120));
    gfx_fill_circle(sun_cx, sun_cy, 52, COLOR_RGB(255, 240, 180));

    /* Distant mountain silhouettes */
    int m_base = (int)sh - 80;
    for (int x = 0; x < (int)sw; x++) {
        int mh = (isin(x / 30) * 35) / 127 + (isin(x / 14) * 15) / 127;
        gfx_fill_rect(x, m_base - mh, 1, (int)sh - (m_base - mh), COLOR_RGB(32, 12, 38));
    }
}

/* 8. Minimal Dark */
static void draw_wp_minimal_dark(void) {
    uint32_t sw = fb_get_width();
    uint32_t sh = fb_get_height();

    /* Solid dark obsidian purple */
    gfx_fill_rect(0, 0, (int)sw, (int)sh, COLOR_RGB(20, 14, 30));

    /* Subtle geometric grid dots spaced every 24px */
    for (int y = 16; y < (int)sh; y += 24) {
        for (int x = 16; x < (int)sw; x += 24) {
            fb_putpixel(x, y, COLOR_ARGB(40, 255, 255, 255));
        }
    }

    /* Subtle glowing border framing */
    gfx_draw_rect(12, 12, (int)sw - 24, (int)sh - 24, COLOR_ARGB(45, 255, 182, 193), 1);
}

/* Wallpaper Registry */
static const wallpaper_t g_wallpapers[WALLPAPER_COUNT] = {
    { "Starry Night",   draw_wp_starry_night },
    { "Sakura Dream",   draw_wp_sakura_dream },
    { "Ocean Twilight", draw_wp_ocean_twilight },
    { "Aurora",         draw_wp_aurora },
    { "Neon City",      draw_wp_neon_city },
    { "Galaxy",         draw_wp_galaxy },
    { "Sunset",         draw_wp_sunset },
    { "Minimal Dark",   draw_wp_minimal_dark }
};

void wallpaper_init(void) {
    uint32_t sw = fb_get_width();
    uint32_t sh = fb_get_height();
    if (sw == 0) sw = DEFAULT_SCREEN_WIDTH;
    if (sh == 0) sh = DEFAULT_SCREEN_HEIGHT;

    if (!g_particles_init) {
        init_particles(sw, sh);
    }
    g_current_wallpaper = 0;
}

void wallpaper_draw(void) {
    if (!g_particles_init) {
        wallpaper_init();
    }

    if (g_current_wallpaper >= 0 && g_current_wallpaper < WALLPAPER_COUNT) {
        if (g_wallpapers[g_current_wallpaper].draw_func) {
            g_wallpapers[g_current_wallpaper].draw_func();
        }
    }
}

void wallpaper_next(void) {
    g_current_wallpaper = (g_current_wallpaper + 1) % WALLPAPER_COUNT;
}

void wallpaper_prev(void) {
    g_current_wallpaper = (g_current_wallpaper - 1 + WALLPAPER_COUNT) % WALLPAPER_COUNT;
}

void wallpaper_set(int index) {
    if (index >= 0 && index < WALLPAPER_COUNT) {
        g_current_wallpaper = index;
    }
}

int wallpaper_get_count(void) {
    return WALLPAPER_COUNT;
}

const char *wallpaper_get_name(int index) {
    if (index >= 0 && index < WALLPAPER_COUNT) {
        return g_wallpapers[index].name;
    }
    return "Unknown";
}

int wallpaper_get_current(void) {
    return g_current_wallpaper;
}
