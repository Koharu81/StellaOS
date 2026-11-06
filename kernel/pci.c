/*
 * StellaOS - pci.c
 * PCI Bus enumeration and hardware detection
 */

#include "types.h"
#include "pci.h"
#include "io.h"
#include "string.h"
#include "serial.h"

static pci_device_t pci_devices[PCI_MAX_DEVICES];
static int pci_device_count = 0;

uint32_t pci_read_config(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)(
        (1U << 31) |
        ((uint32_t)bus << 16) |
        ((uint32_t)dev << 11) |
        ((uint32_t)func << 8) |
        (offset & 0xFC)
    );

    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

void pci_write_config(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address = (uint32_t)(
        (1U << 31) |
        ((uint32_t)bus << 16) |
        ((uint32_t)dev << 11) |
        ((uint32_t)func << 8) |
        (offset & 0xFC)
    );

    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

int pci_get_device_count(void) {
    return pci_device_count;
}

pci_device_t *pci_get_device(int index) {
    if (index >= 0 && index < pci_device_count) {
        return &pci_devices[index];
    }
    return NULL;
}

pci_device_t *pci_find_device(uint16_t vendor, uint16_t device) {
    for (int i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].vendor_id == vendor && pci_devices[i].device_id == device) {
            return &pci_devices[i];
        }
    }
    return NULL;
}

static void pci_scan_function(uint8_t bus, uint8_t dev, uint8_t func) {
    uint32_t reg0 = pci_read_config(bus, dev, func, 0x00);
    uint16_t vendor_id = (uint16_t)(reg0 & 0xFFFF);
    uint16_t device_id = (uint16_t)((reg0 >> 16) & 0xFFFF);

    if (vendor_id == 0xFFFF || vendor_id == 0x0000) {
        return; /* Device does not exist */
    }

    if (pci_device_count >= PCI_MAX_DEVICES) {
        return; /* Device list full */
    }

    uint32_t reg8 = pci_read_config(bus, dev, func, 0x08);
    uint8_t prog_if = (uint8_t)((reg8 >> 8) & 0xFF);
    uint8_t subclass = (uint8_t)((reg8 >> 16) & 0xFF);
    uint8_t class_code = (uint8_t)((reg8 >> 24) & 0xFF);

    uint32_t regC = pci_read_config(bus, dev, func, 0x0C);
    uint8_t header_type = (uint8_t)((regC >> 16) & 0xFF);

    pci_device_t *d = &pci_devices[pci_device_count++];
    d->bus = bus;
    d->device = dev;
    d->function = func;
    d->vendor_id = vendor_id;
    d->device_id = device_id;
    d->class_code = class_code;
    d->subclass = subclass;
    d->prog_if = prog_if;
    d->header_type = header_type;

    /* Read Base Address Registers (BAR0 to BAR5) for standard type 0 headers */
    if ((header_type & 0x7F) == 0x00) {
        for (int bar_idx = 0; bar_idx < 6; bar_idx++) {
            d->bar[bar_idx] = pci_read_config(bus, dev, func, (uint8_t)(0x10 + (bar_idx * 4)));
        }
    } else {
        memset(d->bar, 0, sizeof(d->bar));
    }

    serial_printf("PCI: [%02x:%02x.%d] Vendor=0x%04x Device=0x%04x Class=0x%02x Sub=0x%02x ProgIF=0x%02x\n",
                  bus, dev, func, vendor_id, device_id, class_code, subclass, prog_if);
}

void pci_init(void) {
    pci_device_count = 0;
    memset(pci_devices, 0, sizeof(pci_devices));

    serial_printf("PCI: Scanning PCI bus hierarchy...\n");

    /* Check if host controller is multi-function */
    uint32_t host_c = pci_read_config(0, 0, 0, 0x0C);
    uint8_t host_header = (uint8_t)((host_c >> 16) & 0xFF);
    uint8_t max_bus = (host_header & 0x80) ? 8 : 1;

    for (uint16_t bus = 0; bus < max_bus; bus++) {
        for (uint8_t dev = 0; dev < 32; dev++) {
            uint32_t reg0 = pci_read_config((uint8_t)bus, dev, 0, 0x00);
            uint16_t vendor = (uint16_t)(reg0 & 0xFFFF);
            if (vendor == 0xFFFF || vendor == 0x0000) {
                continue;
            }

            pci_scan_function((uint8_t)bus, dev, 0);

            uint32_t regC = pci_read_config((uint8_t)bus, dev, 0, 0x0C);
            uint8_t header = (uint8_t)((regC >> 16) & 0xFF);

            /* If multi-function device, scan functions 1 to 7 */
            if (header & 0x80) {
                for (uint8_t func = 1; func < 8; func++) {
                    pci_scan_function((uint8_t)bus, dev, func);
                }
            }
        }
    }

    serial_printf("PCI: Enumeration complete (%d devices found)\n", pci_device_count);
}
