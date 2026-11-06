/*
 * StellaOS - multiboot2.h
 * Multiboot2 specification structures and constants
 */

#ifndef STELLA_MULTIBOOT2_H
#define STELLA_MULTIBOOT2_H

#include "types.h"

/* Multiboot2 magic values */
#define MULTIBOOT2_HEADER_MAGIC     0xE85250D6
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289

/* Multiboot2 header tag types */
#define MULTIBOOT2_TAG_END              0
#define MULTIBOOT2_TAG_INFO_REQUEST     1
#define MULTIBOOT2_TAG_ADDRESS          2
#define MULTIBOOT2_TAG_ENTRY            3
#define MULTIBOOT2_TAG_FLAGS            4
#define MULTIBOOT2_TAG_FRAMEBUFFER      5
#define MULTIBOOT2_TAG_MODULE_ALIGN     6

/* Multiboot2 info tag types */
#define MULTIBOOT2_INFO_END             0
#define MULTIBOOT2_INFO_CMDLINE         1
#define MULTIBOOT2_INFO_BOOTLOADER      2
#define MULTIBOOT2_INFO_MODULES         3
#define MULTIBOOT2_INFO_BASIC_MEM       4
#define MULTIBOOT2_INFO_BOOTDEV         5
#define MULTIBOOT2_INFO_MMAP            6
#define MULTIBOOT2_INFO_VBE             7
#define MULTIBOOT2_INFO_FRAMEBUFFER     8
#define MULTIBOOT2_INFO_ELF_SECTIONS    9
#define MULTIBOOT2_INFO_APM             10
#define MULTIBOOT2_INFO_EFI32           11
#define MULTIBOOT2_INFO_EFI64           12
#define MULTIBOOT2_INFO_SMBIOS          13
#define MULTIBOOT2_INFO_ACPI_OLD        14
#define MULTIBOOT2_INFO_ACPI_NEW        15
#define MULTIBOOT2_INFO_NETWORK         16

/* Multiboot2 memory map entry types */
#define MULTIBOOT2_MMAP_AVAILABLE       1
#define MULTIBOOT2_MMAP_RESERVED        2
#define MULTIBOOT2_MMAP_ACPI_RECLAIMABLE 3
#define MULTIBOOT2_MMAP_NVS             4
#define MULTIBOOT2_MMAP_BADRAM          5

/* Framebuffer types */
#define MULTIBOOT2_FB_INDEXED           0
#define MULTIBOOT2_FB_RGB               1
#define MULTIBOOT2_FB_EGA_TEXT          2

/* Multiboot2 info tag header */
struct multiboot2_tag {
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

/* Framebuffer info tag */
struct multiboot2_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
    uint16_t reserved;
    /* Color info follows for RGB type */
    uint8_t  red_field_position;
    uint8_t  red_mask_size;
    uint8_t  green_field_position;
    uint8_t  green_mask_size;
    uint8_t  blue_field_position;
    uint8_t  blue_mask_size;
} __attribute__((packed));

/* Memory map tag */
struct multiboot2_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    /* entries follow */
} __attribute__((packed));

/* Memory map entry */
struct multiboot2_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
} __attribute__((packed));

/* Command line tag */
struct multiboot2_tag_string {
    uint32_t type;
    uint32_t size;
    char     string[];
} __attribute__((packed));

/* Boot info header (passed by bootloader) */
struct multiboot2_info {
    uint32_t total_size;
    uint32_t reserved;
    /* tags follow */
} __attribute__((packed));

#endif /* STELLA_MULTIBOOT2_H */

