/*
 * StellaOS - dma.h
 * ISA DMA (Intel 8237) controller interface
 */

#ifndef STELLA_DMA_H
#define STELLA_DMA_H

#include "types.h"

/* DMA Controller Channels */
#define DMA_CHANNEL_0  0
#define DMA_CHANNEL_1  1
#define DMA_CHANNEL_2  2   /* Standard Floppy disk */
#define DMA_CHANNEL_3  3
#define DMA_CHANNEL_4  4   /* Cascade for controller 2 */
#define DMA_CHANNEL_5  5
#define DMA_CHANNEL_6  6
#define DMA_CHANNEL_7  7

/* DMA Public API */
void dma_init(void);
void dma_set_channel(uint8_t channel, uint32_t address, uint16_t count, bool read);
void dma_enable_channel(uint8_t channel);
void dma_disable_channel(uint8_t channel);

#endif /* STELLA_DMA_H */
