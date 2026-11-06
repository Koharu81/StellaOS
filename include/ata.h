/*
 * StellaOS - ata.h
 * ATA/IDE hard disk driver interface (PIO Mode)
 */

#ifndef STELLA_ATA_H
#define STELLA_ATA_H

#include "types.h"

#define ATA_PRIMARY_IO      0x01F0
#define ATA_PRIMARY_CTRL    0x03F6
#define ATA_SECONDARY_IO    0x0170
#define ATA_SECONDARY_CTRL  0x0376

#define ATA_SECTOR_SIZE     512
#define ATA_MAX_DRIVES      4

/* Drive identifiers */
#define ATA_DRIVE_PRIMARY_MASTER   0
#define ATA_DRIVE_PRIMARY_SLAVE    1
#define ATA_DRIVE_SECONDARY_MASTER 2
#define ATA_DRIVE_SECONDARY_SLAVE  3

/* ATA drive information structure */
typedef struct ata_drive {
    bool present;
    uint8_t drive_id;
    uint16_t io_base;
    uint16_t ctrl_base;
    uint8_t is_slave;
    uint32_t size_sectors;
    char model[41];
} __attribute__((packed)) ata_drive_t;

/* ATA Public API */
void ata_init(void);
bool ata_identify(uint8_t drive);
bool ata_read_sectors(uint8_t drive, uint32_t lba, uint8_t count, void *buf);
bool ata_write_sectors(uint8_t drive, uint32_t lba, uint8_t count, const void *buf);
uint32_t ata_get_disk_size(uint8_t drive);
const char *ata_get_model(uint8_t drive);

#endif /* STELLA_ATA_H */
