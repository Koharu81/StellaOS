/*
 * StellaOS - acpi.c
 * ACPI table detection, power management, shutdown and reboot implementation
 */

#include "types.h"
#include "acpi.h"
#include "io.h"
#include "string.h"
#include "serial.h"

static rsdp_t *cached_rsdp = NULL;
static fadt_t *cached_fadt = NULL;
static uint16_t slp_typa = 0;
static uint16_t slp_typb = 0;
static bool acpi_ready = false;

static bool acpi_validate_checksum(const void *ptr, size_t length) {
    const uint8_t *bytes = (const uint8_t *)ptr;
    uint8_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += bytes[i];
    }
    return (sum == 0);
}

static rsdp_t *acpi_find_rsdp(void) {
    /* 1. Check Extended BIOS Data Area (EBDA) */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
    volatile const uint16_t *bda_ebda = (volatile const uint16_t *)0x040E;
    uint16_t ebda_seg = *bda_ebda;
#pragma GCC diagnostic pop
    uint32_t ebda_addr = ((uint32_t)ebda_seg) << 4;

    if (ebda_addr >= 0x00080000 && ebda_addr < 0x000A0000) {
        for (uint32_t addr = ebda_addr; addr < ebda_addr + 1024; addr += 16) {
            if (memcmp((const void *)addr, "RSD PTR ", 8) == 0) {
                if (acpi_validate_checksum((const void *)addr, 20)) {
                    return (rsdp_t *)addr;
                }
            }
        }
    }

    /* 2. Scan main BIOS ROM memory range 0x000E0000 - 0x000FFFFF */
    for (uint32_t addr = 0x000E0000; addr < 0x00100000; addr += 16) {
        if (memcmp((const void *)addr, "RSD PTR ", 8) == 0) {
            if (acpi_validate_checksum((const void *)addr, 20)) {
                return (rsdp_t *)addr;
            }
        }
    }

    return NULL;
}

static void acpi_parse_s5(void) {
    if (!cached_fadt || cached_fadt->dsdt == 0) {
        return;
    }

    acpi_header_t *dsdt = (acpi_header_t *)cached_fadt->dsdt;
    if (memcmp(dsdt->signature, "DSDT", 4) != 0) {
        return;
    }

    const uint8_t *s = (const uint8_t *)dsdt;
    size_t len = dsdt->length;

    /* Search for AML object "_S5_" */
    for (size_t i = 0; i < len - 8; i++) {
        if (s[i] == '_' && s[i+1] == 'S' && s[i+2] == '5' && s[i+3] == '_') {
            /* Check for package opcode 0x12 */
            size_t p = i + 4;
            if (s[p] == 0x08) p++; /* NameOp */
            if (s[p] == 0x12) {    /* PackageOp */
                p += 3;            /* Skip pkg length and element count */
                if (s[p] == 0x0A) p++; /* BytePrefix */
                slp_typa = (uint16_t)s[p++];
                if (s[p] == 0x0A) p++;
                slp_typb = (uint16_t)s[p++];
                serial_printf("ACPI: Found _S5_ object (SLP_TYPa=0x%x, SLP_TYPb=0x%x)\n",
                              slp_typa, slp_typb);
                return;
            }
        }
    }

    /* Default QEMU S5 sleep type values */
    slp_typa = 0;
    slp_typb = 0;
}

void acpi_init(void) {
    cached_rsdp = acpi_find_rsdp();
    if (!cached_rsdp) {
        serial_printf("ACPI: RSDP not found in memory\n");
        acpi_ready = false;
        return;
    }

    serial_printf("ACPI: Found RSDP at 0x%x (OEM: %.6s)\n",
                  (uint32_t)cached_rsdp, cached_rsdp->oem_id);

    rsdt_t *rsdt = (rsdt_t *)cached_rsdp->rsdt_address;
    if (!rsdt || memcmp(rsdt->header.signature, "RSDT", 4) != 0) {
        serial_printf("ACPI: Invalid RSDT table\n");
        acpi_ready = false;
        return;
    }

    if (!acpi_validate_checksum(rsdt, rsdt->header.length)) {
        serial_printf("ACPI: RSDT checksum verification failed\n");
        acpi_ready = false;
        return;
    }

    size_t entries = (rsdt->header.length - sizeof(acpi_header_t)) / 4;
    for (size_t i = 0; i < entries; i++) {
        acpi_header_t *h = (acpi_header_t *)rsdt->pointers[i];
        if (h && memcmp(h->signature, "FACP", 4) == 0) {
            cached_fadt = (fadt_t *)h;
            break;
        }
    }

    if (cached_fadt) {
        serial_printf("ACPI: Found FADT table at 0x%x (PM1a_CNT=0x%x)\n",
                      (uint32_t)cached_fadt, cached_fadt->pm1a_cnt_blk);
        acpi_parse_s5();
        acpi_ready = true;
    } else {
        serial_printf("ACPI: FADT table not found in RSDT\n");
        acpi_ready = false;
    }
}

bool acpi_is_available(void) {
    return acpi_ready;
}

bool acpi_shutdown(void) {
    serial_printf("ACPI: Initiating system shutdown...\n");

    /* 1. Try ACPI PM1a control register if FADT is present */
    if (cached_fadt && cached_fadt->pm1a_cnt_blk != 0) {
        uint16_t pm1a = (uint16_t)cached_fadt->pm1a_cnt_blk;
        uint16_t pm1b = (uint16_t)cached_fadt->pm1b_cnt_blk;
        uint16_t slp_en = (1 << 13); /* Bit 13: Sleep Enable */

        outw(pm1a, (uint16_t)((slp_typa << 10) | slp_en));
        if (pm1b != 0) {
            outw(pm1b, (uint16_t)((slp_typb << 10) | slp_en));
        }

        io_wait();
    }

    /* 2. QEMU / Bochs modern debug shutdown port */
    outw(0x0604, 0x2000);
    io_wait();

    /* 3. QEMU / Bochs legacy shutdown port */
    outw(0xB004, 0x2000);
    io_wait();

    /* 4. VirtualBox shutdown port */
    outw(0x4004, 0x3400);
    io_wait();

    /* 5. Cloud-hypervisor / QEMU ISA debug exit */
    outb(0x0501, 0x00);

    /* If execution reaches here, halt the CPU */
    __asm__ volatile ("cli");
    while (1) {
        __asm__ volatile ("hlt");
    }

    return true;
}

bool acpi_reboot(void) {
    serial_printf("ACPI: Initiating system reboot...\n");

    /* 1. Wait for 8042 keyboard controller input buffer to empty */
    uint32_t timeout = 100000;
    while (timeout--) {
        if (!(inb(0x64) & 0x02)) {
            break;
        }
        io_wait();
    }

    /* 2. Pulse CPU reset line via 8042 controller (command 0xFE) */
    outb(0x64, 0xFE);
    io_wait();

    /* 3. Try ACPI Reset Register if supported by FADT (ACPI 2.0+) */
    if (cached_fadt && (cached_fadt->flags & (1 << 19))) {
        /* Check if reset register is System I/O (space ID 1) */
        if (cached_fadt->reset_reg[0] == 1) {
            uint16_t port = *(const uint16_t *)(&cached_fadt->reset_reg[4]);
            if (port != 0) {
                outb(port, cached_fadt->reset_value);
                io_wait();
            }
        }
    }

    /* 4. Fallback: Force CPU triple fault by loading an invalid zero-limit IDT */
    struct {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed)) null_idt = { 0, 0 };

    __asm__ volatile (
        "lidt %0\n\t"
        "int $3\n\t"
        :
        : "m"(null_idt)
    );

    /* In case triple fault is trapped, enter infinite halt loop */
    __asm__ volatile ("cli");
    while (1) {
        __asm__ volatile ("hlt");
    }

    return true;
}
