// SPDX-License-Identifier: BSD-3-Clause

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/param.h>
#include <sys/mman.h>
#include "osmem.h"
#include "block_meta.h"

#define MMAP_THRESHOLD  (128 * 1024)
#define METADATA_SIZE   (sizeof(block_meta))
#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))
#define BLOCK_SIZE (ALIGN(sizeof(block_meta)))

block_meta *heap_start;
int heap_preallocated = 0;

block_meta *find_last() {
    block_meta *iter = heap_start;
    block_meta *last = NULL;

    while (iter != NULL) {
        last = iter;
        iter = iter->next;
    }

    return last;
}

block_meta *coalesce(void *ptr) {
    block_meta *iter = (block_meta *)ptr - 1;
    if (iter->status == STATUS_ALLOC)
    {
        iter->status = STATUS_FREE;
        if (iter->prev && iter->prev->status == STATUS_FREE)
        {
            iter->prev->size += (iter->size + METADATA_SIZE);
            iter->prev->next = iter->next;
            if (iter->next)
            {
                iter->next->prev = iter->prev;
            }
        }
    }

    return iter;
}

block_meta *split_block(block_meta *block, size_t size) {
        block_meta *new_block = (block_meta *)((void *)block + METADATA_SIZE + size);
        new_block->size = block->size - size - METADATA_SIZE;
        new_block->status = STATUS_FREE;
        new_block->prev = block;
        new_block->next = block->next;

        block->size = size;
        block->next = new_block;

        if (new_block->next != NULL) {
            new_block->next->prev = new_block;
        }

        return block;
    }

block_meta *find_best(size_t size) {
    block_meta *current = heap_start;
    block_meta *best = NULL;

    while (current != NULL) {
        if (current->status == STATUS_FREE && current->size >= size) {
            if (best == NULL || current->size < best->size) {
                best = current;
            }
        }
        current = current->next;
    }

    return best;
}

void *os_malloc(size_t size)
{
	if (size == 0) {
		return NULL;
	}

	size_t total_size = ALIGN(size) + BLOCK_SIZE;
    size_t align = ALIGN(size);

	if (total_size < MMAP_THRESHOLD) {
		if (heap_start == NULL && heap_preallocated == 0) {
			block_meta * heap = sbrk(MMAP_THRESHOLD);

            if (heap == (void * ) - 1) {
              return NULL;
            }

			heap->size = MMAP_THRESHOLD - METADATA_SIZE;
			heap->next = NULL;
			heap->prev = NULL;
			heap->status = STATUS_FREE;
    		heap_preallocated = 1;
			heap_start = heap;
		}

		block_meta *best = find_best(align);
		block_meta *last = find_last();

		if (best == NULL) {
			if (last->status == STATUS_FREE) {
                sbrk(align - last->size);
                last->size = align;
                last->status = STATUS_ALLOC;
                return (void *)(last + 1);
            } else {
                block_meta *new = sbrk(align + METADATA_SIZE);
                if (new == (void *)-1) {
                    return NULL;
                }

                new->size = align;
                new->status = STATUS_ALLOC;
                last->next = new;
                new->prev = last;
                new->next = NULL;
                return (void *)(new + 1);
            }
		} else {
            if (best->size - align >= ALIGN(1) + METADATA_SIZE) {
                best = split_block(best, align);
            }

            best->status = STATUS_ALLOC;
            return (void *)(best + 1);
        }

        return (void *)(best + 1);

	} else {
		block_meta *block = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (block == MAP_FAILED) {
			return NULL;
		}

		block->next = NULL;
		block->prev = NULL;
		block->size = align;
		block->status = STATUS_MAPPED;
		heap_start = block;

		return (void *)(heap_start + 1);
	}
}

void os_free(void *ptr)
{
	if (!ptr)
    {
        return;
    }

    block_meta *iter = coalesce(ptr);

    if (iter->status == STATUS_MAPPED)
    {
        int result = munmap(iter, (iter->size + BLOCK_SIZE));
        DIE(result == -1, "munmap");
    }
}

void *sussy_malloc(size_t size)
{
	if (size == 0) {
		return NULL;
	}

	size_t total_size = ALIGN(size) + BLOCK_SIZE;
    size_t align = ALIGN(size);
    size_t calloc_threshold = (size_t)getpagesize();


	if (total_size < calloc_threshold) {
		if (heap_start == NULL && heap_preallocated == 0) {
			block_meta * heap = sbrk(MMAP_THRESHOLD);

            if (heap == (void * ) - 1) {
              return NULL;
            }

			heap->size = MMAP_THRESHOLD - METADATA_SIZE;
			heap->next = NULL;
			heap->prev = NULL;
			heap->status = STATUS_FREE;
    		heap_preallocated = 1;
			heap_start = heap;
		}

		block_meta *best = find_best(align);
		block_meta *last = find_last();

		if (best == NULL) {
			if (last->status == STATUS_FREE) {
                sbrk(align - last->size);
                last->size = align;
                last->status = STATUS_ALLOC;
                return (void *)(last + 1);
            } else {
                block_meta *new = sbrk(align + METADATA_SIZE);
                if (new == (void *)-1) {
                    return NULL;
                }

                new->size = align;
                new->status = STATUS_ALLOC;
                last->next = new;
                new->prev = last;
                new->next = NULL;
                return (void *)(new + 1);
            }
		} else {
            if (best->size - align >= ALIGN(1) + METADATA_SIZE) {
                best = split_block(best, align);
            }

            best->status = STATUS_ALLOC;
            return (void *)(best + 1);
        }

        return (void *)(best + 1);

	} else {
		block_meta *block = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (block == MAP_FAILED) {
			return NULL;
		}

		block->next = NULL;
		block->prev = NULL;
		block->size = align;
		block->status = STATUS_MAPPED;
		heap_start = block;

		return (void *)(heap_start + 1);
	}
}

void *os_calloc(size_t nmemb, size_t size)
{
    if (nmemb == 0 || size == 0) {
        return NULL;
    }

    size_t total_size = nmemb * size;
    void *ptr = sussy_malloc(total_size);

    if (ptr != NULL) {
        memset(ptr, 0, total_size);
    }

    return ptr;
}

void *os_realloc(void *ptr, size_t size)
{
    if (ptr == NULL) {
        return os_malloc(size);
    }

    return NULL;
}
