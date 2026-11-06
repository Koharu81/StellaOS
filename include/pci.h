/*
 * StellaOS - pci.h
 * PCI Bus enumeration and configuration space access
 */

#ifndef STELLA_PCI_H
#define STELLA_PCI_H

#include "types.h"

#define PCI_CONFIG_ADDRESS  0x0CF8
#define PCI_CONFIG_DATA     0x0CFC
#define PCI_MAX_DEVICES     64

/* PCI Device structure */
typedef struct pci_device {
    uint8_t  bus;
    uint8_t  device;
    uint8_t  function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  class_code;
    uint8_t  subclass;
    uint8_t  prog_if;
    uint8_t  header_type;
    uint32_t bar[6];
} __attribute__((packed)) pci_device_t;

/* PCI API */
void pci_init(void);
uint32_t pci_read_config(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset);
void pci_write_config(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t value);
int pci_get_device_count(void);
pci_device_t *pci_get_device(int index);
pci_device_t *pci_find_device(uint16_t vendor, uint16_t device);

#endif /* STELLA_PCI_H */
