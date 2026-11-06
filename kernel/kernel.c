/*
 * StellaOS - kernel.c
 * Main kernel entry point - initializes all subsystems
 * Copyright (c) 2024 StellaOS Project
 */

#include "types.h"
#include "multiboot2.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "irq.h"
#include "timer.h"
#include "keyboard.h"
#include "mouse.h"
#include "memory.h"
#include "serial.h"
#include "framebuffer.h"
#include "graphics.h"
#include "font.h"
#include "gui.h"
#include "i18n.h"
#include "installer.h"
#include "string.h"
#include "scheduler.h"
#include "vfs.h"
#include "pci.h"
#include "rtc.h"
#include "ata.h"
#include "acpi.h"
#include "clipboard.h"
#include "syscall.h"
#include "dma.h"
#include "network.h"
#include "wallpaper.h"
#include "widgets.h"
#include "notification.h"
#include "contextmenu.h"
#include "lockscreen.h"
#include "crash.h"

/* Boot splash - shows while system initializes */
static void draw_boot_splash(uint32_t *fb, uint32_t w, uint32_t h, uint32_t pitch, const char *msg) {
    /* Fill with dark purple gradient */
    uint32_t row, col;
    for (row = 0; row < h; row++) {
        uint32_t *line = (uint32_t *)((uint8_t *)fb + row * pitch);
        uint8_t r = (uint8_t)(16 + (row * 30) / h);
        uint8_t g = (uint8_t)(9 + (row * 11) / h);
        uint8_t b = (uint8_t)(32 + (row * 30) / h);
        uint32_t color = (0xFF << 24) | (r << 16) | (g << 8) | b;
        for (col = 0; col < w; col++) {
            line[col] = color;
        }
    }
    (void)msg;
}

void kernel_main(uint32_t magic, struct multiboot2_info *mbi) {
    /* ========== Phase 1: Serial Debug ========== */
    serial_init();
    serial_printf("=================================\n");
    serial_printf("  StellaOS v0.1.0 Hoshizora\n");
    serial_printf("  Booting...\n");
    serial_printf("=================================\n");

    /* ========== Phase 2: Verify Multiboot2 ========== */
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        serial_printf("FATAL: Invalid multiboot2 magic: 0x%x\n", magic);
        while(1) __asm__ volatile("hlt");
    }
    serial_printf("[OK] Multiboot2 verified\n");

    /* ========== Phase 3: CPU Descriptor Tables ========== */
    gdt_init();
    serial_printf("[OK] GDT initialized (5 entries)\n");

    idt_init();
    serial_printf("[OK] IDT initialized (256 gates)\n");

    /* ========== Phase 4: Enable Interrupts ========== */
    __asm__ volatile("sti");
    serial_printf("[OK] Interrupts enabled\n");

    /* ========== Phase 5: Timer ========== */
    timer_init(1000);
    serial_printf("[OK] PIT timer @ 1000Hz\n");

    /* ========== Phase 6: Memory Manager ========== */
    memory_init(mbi);
    serial_printf("[OK] Memory: %d MB total, %d MB free\n",
        memory_get_total() / 1024 / 1024,
        memory_get_free() / 1024 / 1024);

    /* ========== Phase 7: Parse Framebuffer ========== */
    struct multiboot2_tag *tag;
    struct multiboot2_tag_framebuffer *fb_tag = NULL;

    tag = (struct multiboot2_tag *)((uint8_t *)mbi + 8);
    while (tag->type != MULTIBOOT2_INFO_END) {
        if (tag->type == MULTIBOOT2_INFO_FRAMEBUFFER) {
            fb_tag = (struct multiboot2_tag_framebuffer *)tag;
            break;
        }
        uint32_t size = tag->size;
        if (size % 8 != 0)
            size += 8 - (size % 8);
        tag = (struct multiboot2_tag *)((uint8_t *)tag + size);
    }

    if (!fb_tag) {
        serial_printf("FATAL: No framebuffer from bootloader\n");
        while(1) __asm__ volatile("hlt");
    }

    serial_printf("[OK] Framebuffer: %dx%dx%d @ 0x%x (pitch=%d)\n",
        fb_tag->framebuffer_width, fb_tag->framebuffer_height,
        fb_tag->framebuffer_bpp, (uint32_t)fb_tag->framebuffer_addr,
        fb_tag->framebuffer_pitch);

    /* Show boot splash on raw framebuffer */
    draw_boot_splash(
        (uint32_t *)(uint32_t)fb_tag->framebuffer_addr,
        fb_tag->framebuffer_width, fb_tag->framebuffer_height,
        fb_tag->framebuffer_pitch, "Loading StellaOS...");

    /* ========== Phase 8: Framebuffer Driver ========== */
    fb_init((uint32_t *)(uint32_t)fb_tag->framebuffer_addr,
            fb_tag->framebuffer_width,
            fb_tag->framebuffer_height,
            fb_tag->framebuffer_pitch,
            fb_tag->framebuffer_bpp);
    serial_printf("[OK] Double-buffered framebuffer ready\n");

    /* ========== Phase 9: Font ========== */
    font_init();
    serial_printf("[OK] 8x16 bitmap font loaded\n");

    /* ========== Phase 10: Input Devices ========== */
    keyboard_init();
    serial_printf("[OK] PS/2 keyboard driver\n");

    mouse_init();
    mouse_set_bounds((int)fb_tag->framebuffer_width, (int)fb_tag->framebuffer_height);
    serial_printf("[OK] PS/2 mouse driver\n");

    /* ========== Phase 11: System Services ========== */
    clipboard_init();
    serial_printf("[OK] Clipboard service\n");

    rtc_init();
    serial_printf("[OK] RTC real-time clock\n");

    dma_init();
    serial_printf("[OK] DMA controller\n");

    /* ========== Phase 12: Bus & Disk ========== */
    pci_init();
    serial_printf("[OK] PCI bus: %d devices found\n", pci_get_device_count());

    ata_init();
    serial_printf("[OK] ATA/IDE disk driver\n");

    /* ========== Phase 13: Power Management ========== */
    acpi_init();
    serial_printf("[OK] ACPI %s\n", acpi_is_available() ? "available" : "not found");

    /* ========== Phase 14: Filesystem ========== */
    vfs_init();
    serial_printf("[OK] VFS initialized with ramfs\n");

    /* ========== Phase 15: Scheduler ========== */
    scheduler_init();
    serial_printf("[OK] Cooperative scheduler\n");

    /* ========== Phase 16: System Calls ========== */
    syscall_init();
    serial_printf("[OK] System calls (int 0x80)\n");

    /* ========== Phase 17: Network ========== */
    network_init();
    serial_printf("[OK] Network stack\n");

    /* ========== Phase 18: Internationalization ========== */
    i18n_init();
    serial_printf("[OK] i18n: %d languages\n", i18n_get_language_count());

    /* ========== Phase 19: GUI Subsystems ========== */
    wallpaper_init();
    serial_printf("[OK] Wallpaper manager: %d wallpapers\n", wallpaper_get_count());

    notification_init();
    serial_printf("[OK] Notification system\n");

    contextmenu_init();
    serial_printf("[OK] Context menu system\n");

    lockscreen_init();
    serial_printf("[OK] Lock screen\n");

    widgets_init();
    serial_printf("[OK] Desktop widgets\n");

    /* ========== Phase 20: Window Manager ========== */
    gui_init();
    serial_printf("[OK] Window manager & desktop\n");

    /* ========== Boot Complete ========== */
    serial_printf("=================================\n");
    serial_printf("  StellaOS boot complete!\n");
    serial_printf("  Entering desktop...\n");
    serial_printf("=================================\n");

    /* Show welcome notification */
    notification_show("StellaOS", "Welcome to StellaOS! *", 3);

    /* Enter GUI main loop (never returns) */
    gui_main_loop();

    /* Should never reach here */
    while(1) __asm__ volatile("hlt");
}
