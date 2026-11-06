/*
 * StellaOS - memory.c
 * Bitmap physical memory manager and kernel heap first-fit allocator
 */

#include "types.h"
#include "memory.h"
#include "string.h"
#include "multiboot2.h"

#define MAX_PHYSICAL_FRAMES 1048576 /* 4GB / 4KB */
#define BITMAP_WORDS        (MAX_PHYSICAL_FRAMES / 32)
#define HEAP_MAGIC          0x5354454C /* 'STEL' */

/* Bitmap: 1 = used/reserved, 0 = free */
static uint32_t pmm_bitmap[BITMAP_WORDS];
static uint32_t total_memory_bytes = 0;
static uint32_t total_frames = 0;
static uint32_t free_frames = 0;

/* Heap block header */
typedef struct heap_block {
    uint32_t magic;
    size_t   size;
    bool     is_free;
    struct heap_block *next;
    struct heap_block *prev;
} heap_block_t;

static heap_block_t *heap_start = NULL;

static inline void pmm_set_bit(uint32_t frame) {
    if (frame < MAX_PHYSICAL_FRAMES) {
        pmm_bitmap[frame / 32] |= (1U << (frame % 32));
    }
}

static inline void pmm_clear_bit(uint32_t frame) {
    if (frame < MAX_PHYSICAL_FRAMES) {
        pmm_bitmap[frame / 32] &= ~(1U << (frame % 32));
    }
}

static inline bool pmm_test_bit(uint32_t frame) {
    if (frame < MAX_PHYSICAL_FRAMES) {
        return (pmm_bitmap[frame / 32] & (1U << (frame % 32))) != 0;
    }
    return true;
}

static void pmm_reserve_range(uint32_t start_addr, uint32_t end_addr) {
    uint32_t start_frame = start_addr / PAGE_SIZE;
    uint32_t end_frame = ALIGN_UP(end_addr, PAGE_SIZE) / PAGE_SIZE;
    if (end_frame > MAX_PHYSICAL_FRAMES) end_frame = MAX_PHYSICAL_FRAMES;

    for (uint32_t f = start_frame; f < end_frame; f++) {
        if (!pmm_test_bit(f)) {
            pmm_set_bit(f);
            if (free_frames > 0) {
                free_frames--;
            }
        }
    }
}

uint32_t pmm_alloc_frame(void) {
    for (uint32_t i = 0; i < BITMAP_WORDS; i++) {
        if (pmm_bitmap[i] != 0xFFFFFFFFU) {
            for (uint32_t bit = 0; bit < 32; bit++) {
                if (!(pmm_bitmap[i] & (1U << bit))) {
                    uint32_t frame = (i * 32) + bit;
                    pmm_set_bit(frame);
                    if (free_frames > 0) {
                        free_frames--;
                    }
                    return frame * PAGE_SIZE;
                }
            }
        }
    }
    return 0; /* Out of physical memory */
}

void pmm_free_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr / PAGE_SIZE;
    if (frame < MAX_PHYSICAL_FRAMES && pmm_test_bit(frame)) {
        pmm_clear_bit(frame);
        free_frames++;
    }
}

static void heap_init(void) {
    heap_start = (heap_block_t *)KERNEL_HEAP_START;
    heap_start->magic   = HEAP_MAGIC;
    heap_start->size    = KERNEL_HEAP_SIZE - sizeof(heap_block_t);
    heap_start->is_free = true;
    heap_start->next    = NULL;
    heap_start->prev    = NULL;
}

void memory_init(struct multiboot2_info *mbi) {
    /* Initially mark all physical pages as reserved */
    memset(pmm_bitmap, 0xFF, sizeof(pmm_bitmap));
    free_frames = 0;
    total_memory_bytes = 0;

    if (mbi != NULL) {
        /* Parse Multiboot2 information tags (tags are 8-byte aligned) */
        struct multiboot2_tag *tag;
        for (tag = (struct multiboot2_tag *)((uint8_t *)mbi + 8);
             tag->type != MULTIBOOT2_INFO_END;
             tag = (struct multiboot2_tag *)((uint8_t *)tag + ALIGN_UP(tag->size, 8))) {

            if (tag->type == MULTIBOOT2_INFO_MMAP) {
                struct multiboot2_tag_mmap *mmap = (struct multiboot2_tag_mmap *)tag;
                uint8_t *entry_ptr = (uint8_t *)mmap + sizeof(struct multiboot2_tag_mmap);
                uint8_t *end_ptr = (uint8_t *)mmap + mmap->size;

                while (entry_ptr < end_ptr) {
                    struct multiboot2_mmap_entry *entry = (struct multiboot2_mmap_entry *)entry_ptr;

                    if (entry->type == MULTIBOOT2_MMAP_AVAILABLE) {
                        uint64_t start = entry->addr;
                        uint64_t len = entry->len;
                        uint64_t end = start + len;

                        if (end > (uint64_t)total_memory_bytes) {
                            total_memory_bytes = (uint32_t)(end > 0xFFFFFFFFULL ? 0xFFFFFFFFULL : end);
                        }

                        uint32_t start_f = (uint32_t)(start / PAGE_SIZE);
                        uint32_t end_f = (uint32_t)(end / PAGE_SIZE);
                        if (end_f > MAX_PHYSICAL_FRAMES) end_f = MAX_PHYSICAL_FRAMES;

                        for (uint32_t f = start_f; f < end_f; f++) {
                            if (pmm_test_bit(f)) {
                                pmm_clear_bit(f);
                                free_frames++;
                            }
                        }
                    }

                    entry_ptr += mmap->entry_size;
                }
            }
        }
    }

    if (total_memory_bytes == 0) {
        /* Fallback if multiboot mmap tag was unavailable: 128 MB */
        total_memory_bytes = 128 * 1024 * 1024;
        uint32_t max_f = total_memory_bytes / PAGE_SIZE;
        for (uint32_t f = 0; f < max_f; f++) {
            pmm_clear_bit(f);
            free_frames++;
        }
    }

    total_frames = total_memory_bytes / PAGE_SIZE;
    if (total_frames > MAX_PHYSICAL_FRAMES) {
        total_frames = MAX_PHYSICAL_FRAMES;
    }

    /* Reserve 0 to 1MB (Real-mode IVT, BIOS data area, video RAM, etc.) */
    pmm_reserve_range(0x00000000, 0x00100000);

    /* Reserve Kernel memory and heap: 1MB to 20MB (KERNEL_HEAP_END) */
    pmm_reserve_range(0x00100000, KERNEL_HEAP_END);

    /* Reserve Multiboot2 info structure */
    if (mbi != NULL) {
        pmm_reserve_range((uint32_t)mbi, (uint32_t)mbi + mbi->total_size);
    }

    /* Initialize the kernel heap */
    heap_init();
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;

    /* Align size to 8 bytes with a minimum payload size of 16 bytes */
    size = ALIGN_UP(size, 8);
    if (size < 16) size = 16;

    heap_block_t *curr = heap_start;
    while (curr != NULL) {
        if (curr->magic != HEAP_MAGIC) {
            /* Heap corruption detected */
            return NULL;
        }

        if (curr->is_free && curr->size >= size) {
            /* Check if we can split this block */
            if (curr->size >= size + sizeof(heap_block_t) + 16) {
                heap_block_t *split = (heap_block_t *)((uint8_t *)(curr + 1) + size);
                split->magic   = HEAP_MAGIC;
                split->size    = curr->size - size - sizeof(heap_block_t);
                split->is_free = true;
                split->next    = curr->next;
                split->prev    = curr;

                if (split->next != NULL) {
                    split->next->prev = split;
                }

                curr->next = split;
                curr->size = size;
            }

            curr->is_free = false;
            return (void *)(curr + 1);
        }

        curr = curr->next;
    }

    return NULL; /* Out of heap memory */
}

void kfree(void *ptr) {
    if (!ptr) return;

    heap_block_t *block = (heap_block_t *)ptr - 1;
    if (block->magic != HEAP_MAGIC) {
        /* Not a valid heap block */
        return;
    }

    block->is_free = true;

    /* Coalesce with next block if free */
    if (block->next != NULL && block->next->is_free) {
        block->size += sizeof(heap_block_t) + block->next->size;
        block->next = block->next->next;
        if (block->next != NULL) {
            block->next->prev = block;
        }
    }

    /* Coalesce with previous block if free */
    if (block->prev != NULL && block->prev->is_free) {
        block->prev->size += sizeof(heap_block_t) + block->size;
        block->prev->next = block->next;
        if (block->next != NULL) {
            block->next->prev = block->prev;
        }
    }
}

void *kcalloc(size_t count, size_t size) {
    if (count == 0 || size == 0) return NULL;

    /* Check for multiplication overflow */
    if (size > ((size_t)-1) / count) {
        return NULL;
    }

    size_t total = count * size;
    void *ptr = kmalloc(total);
    if (ptr != NULL) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void *krealloc(void *ptr, size_t new_size) {
    if (!ptr) {
        return kmalloc(new_size);
    }
    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }

    heap_block_t *block = (heap_block_t *)ptr - 1;
    if (block->magic != HEAP_MAGIC) {
        return NULL;
    }

    new_size = ALIGN_UP(new_size, 8);
    if (block->size >= new_size) {
        return ptr;
    }

    void *new_ptr = kmalloc(new_size);
    if (!new_ptr) {
        return NULL;
    }

    size_t copy_size = block->size < new_size ? block->size : new_size;
    memcpy(new_ptr, ptr, copy_size);
    kfree(ptr);
    return new_ptr;
}

uint32_t memory_get_total(void) {
    return total_memory_bytes;
}

uint32_t memory_get_used(void) {
    if (total_frames >= free_frames) {
        return (total_frames - free_frames) * PAGE_SIZE;
    }
    return 0;
}

uint32_t memory_get_free(void) {
    return free_frames * PAGE_SIZE;
}
