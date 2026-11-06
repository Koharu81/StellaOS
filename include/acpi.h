/*
 * StellaOS - acpi.h
 * ACPI table detection, power management, shutdown and reboot interface
 */

#ifndef STELLA_ACPI_H
#define STELLA_ACPI_H

#include "types.h"

/* RSDP (Root System Description Pointer) */
typedef struct rsdp {
    char     signature[8];      /* "RSD PTR " */
    uint8_t  checksum;
    char     oem_id[6];
    uint8_t  revision;
    uint32_t rsdt_address;
} __attribute__((packed)) rsdp_t;

/* Standard ACPI Table Header */
typedef struct acpi_header {
    char     signature[4];
    uint32_t length;
    uint8_t  revision;
    uint8_t  checksum;
    char     oem_id[6];
    char     oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed)) acpi_header_t;

/* RSDT (Root System Description Table) */
typedef struct rsdt {
    acpi_header_t header;
    uint32_t      pointers[1];  /* Dynamically sized table pointer array */
} __attribute__((packed)) rsdt_t;

/* FADT (Fixed ACPI Description Table) */
typedef struct fadt {
    acpi_header_t header;
    uint32_t      firmware_ctrl;
    uint32_t      dsdt;
    uint8_t       reserved;
    uint8_t       preferred_pm_profile;
    uint16_t      sci_int;
    uint32_t      smi_cmd;
    uint8_t       acpi_enable;
    uint8_t       acpi_disable;
    uint8_t       s4bios_req;
    uint8_t       pstate_cnt;
    uint32_t      pm1a_evt_blk;
    uint32_t      pm1b_evt_blk;
    uint32_t      pm1a_cnt_blk;
    uint32_t      pm1b_cnt_blk;
    uint32_t      pm2_cnt_blk;
    uint32_t      pm_tmr_blk;
    uint32_t      gpe0_blk;
    uint32_t      gpe1_blk;
    uint8_t       pm1_evt_len;
    uint8_t       pm1_cnt_len;
    uint8_t       pm2_cnt_len;
    uint8_t       pm_tmr_len;
    uint8_t       gpe0_blk_len;
    uint8_t       gpe1_blk_len;
    uint8_t       gpe1_base;
    uint8_t       cst_cnt;
    uint16_t      p_lvl2_lat;
    uint16_t      p_lvl3_lat;
    uint16_t      flush_size;
    uint16_t      flush_stride;
    uint8_t       duty_offset;
    uint8_t       duty_width;
    uint8_t       day_alarm;
    uint8_t       mon_alarm;
    uint8_t       century;
    uint16_t      boot_flags;
    uint8_t       reserved2;
    uint32_t      flags;
    uint8_t       reset_reg[12];
    uint8_t       reset_value;
} __attribute__((packed)) fadt_t;

/* ACPI Public API */
void acpi_init(void);
bool acpi_shutdown(void);
bool acpi_reboot(void);
bool acpi_is_available(void);

#endif /* STELLA_ACPI_H */
