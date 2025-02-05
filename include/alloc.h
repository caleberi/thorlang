#ifndef _THOR_ALLOC_H_
#define _THOR_ALLOC_H_

// references :
// - https://github.com/rtmacaibay/memory-allocator/blob/master/allocator.c
// - https://jamesgolick.com/2013/5/15/memory-allocators-101.html
// - https://arjunsreedharan.org/post/148675821737/memory-allocators-101-write-a-simple-memory
// - https://wiki.osdev.org/Memory_Allocation
#include "common.h"

#ifndef DEBUG
#define DEBUG 0
#endif
#define LOG(fmt, ...)                                     \
    do                                                    \
    {                                                     \
        if (DEBUG)                                        \
            fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
                    __LINE__, __func__, __VA_ARGS__);     \
    } while (0)

#define LOGP(str)                                       \
    do                                                  \
    {                                                   \
        if (DEBUG)                                      \
            fprintf(stderr, "%s:%d:%s(): %s", __FILE__, \
                    __LINE__, __func__, str);           \
    } while (0)

typedef enum
{
    BEST_FIT,
    WORST_FIT,
    NEXT_FIT,
    FIRST_FIT,
} algorithm;

typedef struct mem_block
{
    // flag for identifying if block is free or not
    bool is_free;
    // Calculating usage stat of page block
    unsigned long usage;
    // For sanity check, each block needs unique id for identifying
    // the block. In event of colasing or splitting memory block
    // then a new magic number is generated.
    unsigned long magic_number; // borrowed from 3 piece text
    // Size of the allocated memory region
    size_t size;
    // Indicates the size of the mapped memory
    size_t region_size;

    // Pointer to the start of the page block mapped memory
    // This simplifies the process of finding where memory mappings begin
    struct mem_block *region_start;

    // Pointer to the start of the next memory block for the mapped memory region
    // This is represented as shown below
    // [ block_1   ]->>>>>[   block_2     ]->>>>
    //                            |
    //                        ||-------||
    //                region_start    region_size(offset)
    // Next page block in the chain
    struct mem_block *next; // 16
} mem_block;

/**
 * print_mem_block
 *
 * Prints out the current page block state, including both the regions and sub-region blocks.
 * Entries are printed in order, so there is an implied link from the topmost
 * entry to the next, and so on.
     Output representation should be
     [REGION] [MEMORY_MAP_OF_SUBPAGE_REGION_START]
     [MEMORY_MAP_OF_SUBPAGE_REGION_END] [PAGE_SIZE]
 */

void print_mem_block();
void print_mem_block_to_file(FILE *);

/*
 * reuse - recursive helper function to determine which of the
 * memory block can be colased and reused
 */
void *reuse(size_t, algorithm);

void *reuse_space(mem_block *, size_t);

void *p_malloc(size_t);
void *p_malloc_fit(size_t, algorithm);

void *p_calloc(size_t, size_t);
void *p_calloc_fit(size_t, size_t, algorithm);

void *p_realloc(size_t, size_t);
void *p_realloc_fit(size_t, size_t, algorithm);

void p_free(void *);

#endif // _THOR_ALLOC_H_