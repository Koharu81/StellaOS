/*
 * StellaOS - memory.h
 * Physical memory management and kernel heap memory allocator
 */

#ifndef STELLA_MEMORY_H
#define STELLA_MEMORY_H

#include "types.h"
#include "multiboot2.h"

void memory_init(struct multiboot2_info *mbi);

/* Kernel dynamic heap allocator */
void *kmalloc(size_t size);
void kfree(void *ptr);
void *kcalloc(size_t count, size_t size);
void *krealloc(void *ptr, size_t new_size);

/* Physical frame allocator */
uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t frame_addr);

/* Memory metrics in bytes */
uint32_t memory_get_total(void);
uint32_t memory_get_used(void);
uint32_t memory_get_free(void);

#endif /* STELLA_MEMORY_H */
