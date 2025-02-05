#include "alloc.h"

// Start (head) of the list
mem_block *l_head = NULL;

// End (tail) part of the free list
mem_block *l_tail = NULL;

mem_block *next_fit_curr = NULL;

// Allocation tracker
unsigned long magic_number_allocation = 0;

// interprocess mutex
pthread_mutex_t magic_number_allocation_mutex = PTHREAD_MUTEX_INITIALIZER;

void print_mem_block() { print_mem_block_to_file(stdout); }

void print_mem_block_to_file(FILE *fp)
{
    fprintf(fp, "=== Current Memory State ===");
    mem_block *current_block = l_head;
    mem_block *current_region = NULL;
    while (current_block != NULL)
    {
        if (current_block->region_start != current_region)
        {
            current_region = current_block->region_start;
            fprintf(fp,
                    "[REGION] %p-%p %zu\n",
                    current_region,
                    (void *)current_region->region_start + current_region->region_size,
                    current_region->size);
        }
        fprintf(fp,
                "[BLOCK]  %p-%p (%ld) %zu %zu  %s\n",
                current_block,
                (void *)current_block + current_block->size,
                current_block->magic_number,
                current_block->size,
                !(current_block->usage == 0) ? 0
                                             : current_block->usage - sizeof(struct mem_block),
                current_block->is_free ? "true" : "false");
        current_block = current_block->next;
    }
}

void *reuse_space(mem_block *ptr, size_t real_size)
{
    if (ptr->usage == 0)
    {
        unsigned long old_id = ptr->magic_number;
        ptr->usage = real_size;
        ptr->magic_number = magic_number_allocation++;
        LOG("Allocation request USING reuse; reusing alloc %ld\n", old_id);
        return ptr + 1;
    }

    mem_block *free_block = (void *)ptr + ptr->usage; // starting point of this block;
    free_block->magic_number = magic_number_allocation++;
    free_block->usage = real_size;
    free_block->region_start = ptr->region_start;
    free_block->region_size = ptr->region_size;
    free_block->size = ptr->size - ptr->usage;

    if (l_tail == ptr)
    {
        free_block->next = NULL;
        l_tail->next = free_block;
        l_tail = free_block;
    }

    if (l_tail != ptr)
    {
        free_block->next = ptr->next;
        ptr->next = free_block;
    }

    ptr->size = ptr->usage;

    LOG("Allocation request USING reuse;\n size = %zu, alloc = %ld, where?: %ld\n",
        real_size, free_block->magic_number, ptr->magic_number);
    return free_block + 1;
}

void *reuse(size_t size, algorithm algorithm)
{
    if (l_head == NULL)
        return NULL;

    size_t fits_best = SIZE_MAX;
    size_t fits_worst = 0;

    mem_block *spot_to_allocate = NULL;
    mem_block *current;

    size_t real_size = size + sizeof(mem_block);
    if (algorithm == NEXT_FIT)
        current = next_fit_curr != NULL ? next_fit_curr : l_head;
    else
        current = l_head;

#define CASE_WRAP(algorithm, condition) \
    case algorithm:                     \
        condition

    while (current != NULL)
    {

        // Figure out how much space is free in the current
        // page block so we can figure out the block to use
        size_t free_space = current->size - current->usage;
        switch (algorithm)
        {
            CASE_WRAP(FIRST_FIT, if (current->usage < current->size && real_size <= free_space) { return reuse_space(current, real_size); });

            CASE_WRAP(BEST_FIT, if (current->usage < current->size && real_size <= free_space) {
                if (free_space < fits_best) {
                    // select the smallest free space
                    fits_best = free_space; 
                    spot_to_allocate = current;
                } });

            CASE_WRAP(WORST_FIT, if (current->usage < current->size && real_size <= free_space) {   
                if (free_space > fits_worst) {   
                     // select the largest free space                                
                    fits_worst = free_space;                                 
                    spot_to_allocate = current;                                
                } });

            CASE_WRAP(NEXT_FIT, if (current->usage < current->size && real_size <= free_space) { 
                next_fit_curr = current->next ? current->next : l_head;
                return reuse_space(current, real_size); });
        }
        current = current->next;
    }
#undef CASE_WRAP
    if (spot_to_allocate != NULL)
        return reuse_space(spot_to_allocate, real_size);
    return NULL;
}

void *p_malloc(size_t size) { return p_malloc_fit(size, BEST_FIT); }
void *p_malloc_fit(size_t size, algorithm algorithm)
{
    pthread_mutex_lock(&magic_number_allocation_mutex);

    if (size % 8 != 0)
        size = (((int)size / 8) * 8) + 8;

    void *ptr = reuse(size, algorithm);

    if (ptr != NULL)
    {
        pthread_mutex_unlock(&magic_number_allocation_mutex);
        return ptr;
    }

    size_t real_size = sizeof(mem_block) + size;

    int page_size = getpagesize();

    size_t number_of_pages = real_size / page_size;

    // if we have a remainder , we need +1 numberof page
    if (real_size % page_size)
        number_of_pages++;

    size_t region_size = number_of_pages * page_size;
    mem_block *block = mmap(
        NULL,
        region_size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0);

    if (block == MAP_FAILED)
    {
        pthread_mutex_unlock(&magic_number_allocation_mutex);
        perror("mmap");
        return NULL;
    }

    block->magic_number = magic_number_allocation++;
    block->usage = region_size;
    block->next = NULL;
    block->size = real_size;
    block->is_free = false;
    block->region_start = block;

    if (l_head == NULL)
    {
        l_head = block;
        l_tail = block;
    }
    else
    {
        l_tail->next = block;
        l_tail = block;
        l_tail->next = NULL;
    }

    pthread_mutex_unlock(&magic_number_allocation_mutex);
    return block + 1;
}

void *p_calloc(size_t count, size_t size) { return p_calloc_fit(count, size, BEST_FIT); }
void *p_calloc_fit(size_t count, size_t size, algorithm algorithm)
{
    if (count == 0 || size == 0)
        return NULL;
    void *ptr = p_malloc_fit(count * size, algorithm);
    memset(ptr, 0, count * size);
    return ptr;
}

static void destroy(mem_block *ptr)
{
    int ret = munmap(ptr->region_start, ptr->region_size);
    if (ret == -1)
    {
        perror("munmap");
    }
    LOGP("Free request; Destroyed a pointer\n");
}

void p_free(void *ptr)
{
    pthread_mutex_lock(&magic_number_allocation_mutex);
    if (ptr == NULL)
    {
        pthread_mutex_unlock(&magic_number_allocation_mutex);
        return;
    }

    mem_block *block = (mem_block *)ptr - 1;
    LOG("Free request; size = %zu, alloc = %lu\n", block->usage, block->magic_number);

    block->usage = 0;
    mem_block *original_region_start = block->region_start;
    mem_block *prev_block = l_head;
    mem_block *first_half = NULL;
    mem_block *second_half = NULL;

    bool is_first_node = false;

    if (prev_block->region_start == original_region_start)
    {
        is_first_node = true;
        if (prev_block->usage != 0)
        {
            pthread_mutex_unlock(&magic_number_allocation_mutex);
            return;
        }
    }

    while (prev_block->next != NULL)
    {
        mem_block *current = prev_block->next;
        if (current->region_start == original_region_start)
        {
            if (first_half == NULL)
            {
                first_half = prev_block;
                continue;
            }
            if (current->usage != 0)
            {
                LOGP("Free request; Completed - block still in use\n");
                pthread_mutex_unlock(&magic_number_allocation_mutex);
                return;
            }
        }
        else if (first_half != NULL)
        {
            second_half = current;
            break;
        }
        prev_block = prev_block->next;
    }

    if (is_first_node)
    {
        if (second_half == NULL)
        {
            destroy(block);
            l_head = NULL;
            l_tail = NULL;
        }
        else
        {

            destroy(block);
            l_head = second_half;
        }
    }
    else if (first_half != NULL)
    {
        if (second_half == NULL)
        {
            destroy(block);
            l_tail = first_half;
            l_tail->next = NULL;
        }
        else
        {
            destroy(block);
            first_half->next = second_half;
        }
    }
    else
    {
        perror("free");
    }

    pthread_mutex_unlock(&magic_number_allocation_mutex);
    LOGP("Free request: completed\n");
}

void *realloc(void *ptr, size_t size)
{
    if (size % 8 != 0)
        size = (((int)size / 8) * 8) + 8;

    if (ptr == NULL)
        return p_malloc(size);

    if (size == 0)
    {
        p_free(ptr);
        return NULL;
    }

    mem_block *block = (mem_block *)ptr - 1;
    if (block->size >= size)
    {
        block->usage = size;
        return ptr;
    }
    else
    {
        void *new = p_malloc(size);
        memcpy(new, ptr, block->usage);
        free(ptr);
        return new;
    }

    return NULL;
}