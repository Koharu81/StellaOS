/*
 * StellaOS - dma.c
 * ISA DMA (Intel 8237) controller driver implementation
 */

#include "types.h"
#include "dma.h"
#include "io.h"
#include "serial.h"

/* Primary Controller (Channels 0-3, 8-bit) */
#define DMA1_STATUS_REG     0x08
#define DMA1_COMMAND_REG    0x08
#define DMA1_REQUEST_REG    0x09
#define DMA1_MASK_REG       0x0A
#define DMA1_MODE_REG       0x0B
#define DMA1_CLEAR_FF_REG   0x0C
#define DMA1_RESET_REG      0x0D
#define DMA1_CLEAR_MASK_REG 0x0E
#define DMA1_MASK_ALL_REG   0x0F

/* Secondary Controller (Channels 4-7, 16-bit) */
#define DMA2_STATUS_REG     0xD0
#define DMA2_COMMAND_REG    0xD0
#define DMA2_REQUEST_REG    0xD2
#define DMA2_MASK_REG       0xD4
#define DMA2_MODE_REG       0xD6
#define DMA2_CLEAR_FF_REG   0xD8
#define DMA2_RESET_REG      0xDA
#define DMA2_CLEAR_MASK_REG 0xDC
#define DMA2_MASK_ALL_REG   0xDE

/* Channel Port Tables */
static const uint8_t dma_address_ports[8] = {
    0x00, 0x02, 0x04, 0x06, 0xC0, 0xC4, 0xC8, 0xCC
};

static const uint8_t dma_count_ports[8] = {
    0x01, 0x03, 0x05, 0x07, 0xC2, 0xC6, 0xCA, 0xCE
};

static const uint8_t dma_page_ports[8] = {
    0x87, 0x83, 0x81, 0x82, 0x8F, 0x8B, 0x89, 0x8A
};

void dma_disable_channel(uint8_t channel) {
    if (channel < 4) {
        outb(DMA1_MASK_REG, (uint8_t)(channel | 0x04));
    } else if (channel < 8) {
        outb(DMA2_MASK_REG, (uint8_t)((channel - 4) | 0x04));
    }
}

void dma_enable_channel(uint8_t channel) {
    if (channel < 4) {
        outb(DMA1_MASK_REG, (uint8_t)(channel & 0x03));
    } else if (channel < 8) {
        /* Unmask cascade channel 4 if enabling 16-bit channel */
        outb(DMA1_MASK_REG, 0x00);
        outb(DMA2_MASK_REG, (uint8_t)((channel - 4) & 0x03));
    }
}

void dma_set_channel(uint8_t channel, uint32_t address, uint16_t count, bool read) {
    if (channel == 4 || channel > 7) {
        return; /* Channel 4 is reserved for cascade; invalid > 7 */
    }

    dma_disable_channel(channel);

    if (channel < 4) {
        /* 8-bit DMA channel (Channels 0-3) */
        uint8_t addr_port = dma_address_ports[channel];
        uint8_t count_port = dma_count_ports[channel];
        uint8_t page_port = dma_page_ports[channel];

        /* Clear byte pointer flip-flop */
        outb(DMA1_CLEAR_FF_REG, 0x00);

        /* Mode byte:
         * Bits 1-0: Channel (0-3)
         * Bits 3-2: Transfer type:
         *   01 (0x04): Write to memory (device -> memory, hardware read)
         *   10 (0x08): Read from memory (memory -> device, hardware write)
         * Bits 5-4: Single-cycle transfer (0x00)
         * Bits 7-6: Single mode (0x40)
         */
        uint8_t mode = (uint8_t)((channel & 0x03) | 0x40 | (read ? 0x04 : 0x08));
        outb(DMA1_MODE_REG, mode);

        /* Set memory base address */
        outb(addr_port, (uint8_t)(address & 0xFF));
        outb(addr_port, (uint8_t)((address >> 8) & 0xFF));

        /* Set page register (bits 16-23) */
        outb(page_port, (uint8_t)((address >> 16) & 0xFF));

        /* Set transfer count (count - 1 bytes) */
        uint16_t actual_count = (uint16_t)(count - 1);
        outb(count_port, (uint8_t)(actual_count & 0xFF));
        outb(count_port, (uint8_t)((actual_count >> 8) & 0xFF));

        /* Clear flip-flop after write */
        outb(DMA1_CLEAR_FF_REG, 0x00);
    } else {
        /* 16-bit DMA channel (Channels 5-7) */
        uint8_t ch = (uint8_t)(channel - 4);
        uint8_t addr_port = dma_address_ports[channel];
        uint8_t count_port = dma_count_ports[channel];
        uint8_t page_port = dma_page_ports[channel];

        /* Clear byte pointer flip-flop on controller 2 */
        outb(DMA2_CLEAR_FF_REG, 0x00);

        /* Mode byte for controller 2 */
        uint8_t mode = (uint8_t)((ch & 0x03) | 0x40 | (read ? 0x04 : 0x08));
        outb(DMA2_MODE_REG, mode);

        /* 16-bit DMA addresses are 16-bit word aligned */
        uint32_t word_addr = address >> 1;
        outb(addr_port, (uint8_t)(word_addr & 0xFF));
        outb(addr_port, (uint8_t)((word_addr >> 8) & 0xFF));

        /* Page register */
        outb(page_port, (uint8_t)((address >> 16) & 0xFE));

        /* Transfer count in 16-bit words */
        uint16_t word_count = (uint16_t)((count >> 1) - 1);
        outb(count_port, (uint8_t)(word_count & 0xFF));
        outb(count_port, (uint8_t)((word_count >> 8) & 0xFF));

        outb(DMA2_CLEAR_FF_REG, 0x00);
    }

    serial_printf("DMA: Configured channel %d (addr: 0x%x, count: %u, dir: %s)\n",
                  channel, address, count, read ? "read" : "write");
}

void dma_init(void) {
    /* Reset both master and slave DMA controllers */
    outb(DMA1_RESET_REG, 0x00);
    outb(DMA2_RESET_REG, 0x00);

    /* Mask all channels initially */
    outb(DMA1_MASK_ALL_REG, 0x0F);
    outb(DMA2_MASK_ALL_REG, 0x0F);

    /* Unmask cascade channel 4 on controller 1 */
    outb(DMA1_MASK_REG, 0x00);

    serial_printf("DMA: Initialized ISA 8237 DMA controllers (8-bit and 16-bit channels)\n");
}
