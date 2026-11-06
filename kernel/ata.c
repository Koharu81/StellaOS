/*
 * StellaOS - ata.c
 * ATA/IDE hard disk driver implementation (PIO Mode)
 */

#include "types.h"
#include "ata.h"
#include "io.h"
#include "string.h"
#include "serial.h"

/* ATA Commands */
#define ATA_CMD_READ_SECTORS   0x20
#define ATA_CMD_WRITE_SECTORS  0x30
#define ATA_CMD_IDENTIFY       0xEC
#define ATA_CMD_CACHE_FLUSH    0xE7

/* ATA Status Register Bits */
#define ATA_SR_ERR  0x01    /* Error */
#define ATA_SR_IDX  0x02    /* Index */
#define ATA_SR_CORR 0x04    /* Corrected data */
#define ATA_SR_DRQ  0x08    /* Data request ready */
#define ATA_SR_DSC  0x10    /* Drive seek complete */
#define ATA_SR_DF   0x20    /* Drive fault */
#define ATA_SR_DRDY 0x40    /* Drive ready */
#define ATA_SR_BSY  0x80    /* Busy */

static ata_drive_t drives[ATA_MAX_DRIVES];

static void ata_delay_400ns(uint16_t ctrl) {
    inb(ctrl);
    inb(ctrl);
    inb(ctrl);
    inb(ctrl);
}

static bool ata_poll_status(uint16_t io, uint16_t ctrl, bool check_drq) {
    ata_delay_400ns(ctrl);

    uint32_t timeout = 100000;
    while (timeout--) {
        uint8_t status = inb(io + 7);

        if (status & ATA_SR_ERR) {
            return false;
        }
        if (status & ATA_SR_DF) {
            return false;
        }
        if (!(status & ATA_SR_BSY)) {
            if (!check_drq || (status & ATA_SR_DRQ)) {
                return true;
            }
        }
    }

    return false;
}

bool ata_identify(uint8_t drive) {
    if (drive >= ATA_MAX_DRIVES) {
        return false;
    }

    ata_drive_t *d = &drives[drive];
    uint16_t io = d->io_base;
    uint16_t ctrl = d->ctrl_base;
    uint8_t slave_bit = d->is_slave;

    /* Select drive */
    outb(io + 6, (uint8_t)(0xA0 | (slave_bit << 4)));
    ata_delay_400ns(ctrl);

    /* Zero out sector count and LBA registers */
    outb(io + 2, 0);
    outb(io + 3, 0);
    outb(io + 4, 0);
    outb(io + 5, 0);

    /* Send IDENTIFY command */
    outb(io + 7, ATA_CMD_IDENTIFY);
    ata_delay_400ns(ctrl);

    /* Check if drive exists */
    uint8_t status = inb(io + 7);
    if (status == 0) {
        d->present = false;
        return false; /* No device */
    }

    /* Wait for BSY to clear with timeout */
    uint32_t timeout = 100000;
    while (timeout--) {
        status = inb(io + 7);
        if (!(status & ATA_SR_BSY)) break;
    }
    if (timeout == 0) {
        d->present = false;
        return false;
    }

    /* Check for ATAPI / SATA */
    uint8_t mid = inb(io + 4);
    uint8_t high = inb(io + 5);
    if ((mid == 0x14 && high == 0xEB) || (mid == 0x3C && high == 0xC3)) {
        d->present = false;
        return false; /* ATAPI device, not ATA hard disk */
    }

    /* Wait for DRQ or ERR */
    timeout = 100000;
    while (timeout--) {
        status = inb(io + 7);
        if (status & ATA_SR_ERR) {
            d->present = false;
            return false;
        }
        if (status & ATA_SR_DRQ) break;
    }
    if (timeout == 0) {
        d->present = false;
        return false;
    }

    /* Read 256 words (512 bytes) of IDENTIFY data */
    uint16_t identify_buf[256];
    for (int i = 0; i < 256; i++) {
        identify_buf[i] = inw(io + 0);
    }

    /* Extract model string (words 27 to 46, 40 bytes) with byte swapping */
    for (int i = 0; i < 20; i++) {
        uint16_t w = identify_buf[27 + i];
        d->model[i * 2]     = (char)((w >> 8) & 0xFF);
        d->model[i * 2 + 1] = (char)(w & 0xFF);
    }
    d->model[40] = '\0';

    /* Trim trailing whitespace from model */
    for (int i = 39; i >= 0; i--) {
        if (d->model[i] == ' ' || d->model[i] == '\0') {
            d->model[i] = '\0';
        } else {
            break;
        }
    }

    /* Extract LBA28 total sector count (words 60-61) */
    d->size_sectors = (uint32_t)identify_buf[60] | ((uint32_t)identify_buf[61] << 16);
    d->present = true;

    serial_printf("ATA: Drive %d [%s] Model: '%s' Size: %u MB (%u sectors)\n",
                  drive,
                  slave_bit ? "Slave" : "Master",
                  d->model,
                  (d->size_sectors * ATA_SECTOR_SIZE) / (1024 * 1024),
                  d->size_sectors);

    return true;
}

void ata_init(void) {
    memset(drives, 0, sizeof(drives));

    /* Initialize drive configurations */
    drives[0].drive_id = 0;
    drives[0].io_base = ATA_PRIMARY_IO;
    drives[0].ctrl_base = ATA_PRIMARY_CTRL;
    drives[0].is_slave = 0;

    drives[1].drive_id = 1;
    drives[1].io_base = ATA_PRIMARY_IO;
    drives[1].ctrl_base = ATA_PRIMARY_CTRL;
    drives[1].is_slave = 1;

    drives[2].drive_id = 2;
    drives[2].io_base = ATA_SECONDARY_IO;
    drives[2].ctrl_base = ATA_SECONDARY_CTRL;
    drives[2].is_slave = 0;

    drives[3].drive_id = 3;
    drives[3].io_base = ATA_SECONDARY_IO;
    drives[3].ctrl_base = ATA_SECONDARY_CTRL;
    drives[3].is_slave = 1;

    serial_printf("ATA: Probing IDE controllers and drives...\n");

    /* Probe drives */
    for (uint8_t i = 0; i < ATA_MAX_DRIVES; i++) {
        ata_identify(i);
    }
}

bool ata_read_sectors(uint8_t drive, uint32_t lba, uint8_t count, void *buf) {
    if (drive >= ATA_MAX_DRIVES || !drives[drive].present || !buf || count == 0) {
        return false;
    }

    ata_drive_t *d = &drives[drive];
    uint16_t io = d->io_base;
    uint16_t ctrl = d->ctrl_base;
    uint8_t slave_bit = d->is_slave;

    /* Wait for controller to be not busy */
    if (!ata_poll_status(io, ctrl, false)) {
        return false;
    }

    /* Drive / Head selection in LBA28 mode */
    outb(io + 6, (uint8_t)(0xE0 | (slave_bit << 4) | ((lba >> 24) & 0x0F)));
    ata_delay_400ns(ctrl);

    outb(io + 1, 0x00);                       /* NULL error / features */
    outb(io + 2, count);                      /* Sector count */
    outb(io + 3, (uint8_t)(lba & 0xFF));       /* LBA 0-7 */
    outb(io + 4, (uint8_t)((lba >> 8) & 0xFF)); /* LBA 8-15 */
    outb(io + 5, (uint8_t)((lba >> 16) & 0xFF));/* LBA 16-23 */
    outb(io + 7, ATA_CMD_READ_SECTORS);

    uint16_t *ptr = (uint16_t *)buf;

    for (uint8_t s = 0; s < count; s++) {
        if (!ata_poll_status(io, ctrl, true)) {
            return false;
        }

        /* Read 256 words (512 bytes) */
        for (int i = 0; i < 256; i++) {
            *ptr++ = inw(io + 0);
        }
    }

    return true;
}

bool ata_write_sectors(uint8_t drive, uint32_t lba, uint8_t count, const void *buf) {
    if (drive >= ATA_MAX_DRIVES || !drives[drive].present || !buf || count == 0) {
        return false;
    }

    ata_drive_t *d = &drives[drive];
    uint16_t io = d->io_base;
    uint16_t ctrl = d->ctrl_base;
    uint8_t slave_bit = d->is_slave;

    if (!ata_poll_status(io, ctrl, false)) {
        return false;
    }

    outb(io + 6, (uint8_t)(0xE0 | (slave_bit << 4) | ((lba >> 24) & 0x0F)));
    ata_delay_400ns(ctrl);

    outb(io + 1, 0x00);
    outb(io + 2, count);
    outb(io + 3, (uint8_t)(lba & 0xFF));
    outb(io + 4, (uint8_t)((lba >> 8) & 0xFF));
    outb(io + 5, (uint8_t)((lba >> 16) & 0xFF));
    outb(io + 7, ATA_CMD_WRITE_SECTORS);

    const uint16_t *ptr = (const uint16_t *)buf;

    for (uint8_t s = 0; s < count; s++) {
        if (!ata_poll_status(io, ctrl, true)) {
            return false;
        }

        /* Write 256 words (512 bytes) */
        for (int i = 0; i < 256; i++) {
            outw(io + 0, *ptr++);
        }
    }

    /* Flush write cache */
    outb(io + 7, ATA_CMD_CACHE_FLUSH);
    ata_poll_status(io, ctrl, false);

    return true;
}

uint32_t ata_get_disk_size(uint8_t drive) {
    if (drive < ATA_MAX_DRIVES && drives[drive].present) {
        return drives[drive].size_sectors;
    }
    return 0;
}

const char *ata_get_model(uint8_t drive) {
    if (drive < ATA_MAX_DRIVES && drives[drive].present) {
        return drives[drive].model;
    }
    return "Unknown";
}
