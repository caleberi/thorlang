#ifndef _THOR_MEMORY_H_
#define _THOR_MEMORY_H_
#include "common.h"

#define RESIZE_PERCENT 80
#define CHUNK_SIZE 10

#define GROW_CAP(type, capacity, percent_growth) \
    ((capacity) > ((type)(percent_growth / 100) * capacity) ? (capacity + (type)(capacity * ((0.5 * percent_growth) / 100))) : 16)

#define GROW_ARRAY(type, pointer, old_cap, new_cap)       \
    (type *)reallocate(pointer, sizeof(type) * (old_cap), \
                       sizeof(type) * (new_cap))

#define FREE_ARRAY(type, pointer, capacity) \
    reallocate(pointer, sizeof(type) * (capacity), 0)

void *reallocate(void *, size_t, size_t);

#define INIT_ARRAY(T, val_type) \
    typedef struct T            \
    {                           \
        int capacity;           \
        int count;              \
        val_type *entries;      \
    } T;

#define GENERIC_ARRAY_OPS(T, val_type) \
    void init_##T(T *);                \
    void write_##T(T *, val_type);     \
    void free_##T(T *);

#define GENERIC_ARRAY_IMPL(T, val_type, name, value)                                      \
    void init_##T(T *name)                                                                \
    {                                                                                     \
        name->count = 0;                                                                  \
        name->capacity = 0;                                                               \
        name->entries = ((void *)0);                                                      \
    }                                                                                     \
    void write_##T(T *name, val_type value)                                               \
    {                                                                                     \
        if (name->capacity < name->count + 1)                                             \
        {                                                                                 \
            int old_cap = name->capacity;                                                 \
            name->capacity = GROW_CAP(int, old_cap, RESIZE_PERCENT);                      \
            name->entries = GROW_ARRAY(val_type, name->entries, old_cap, name->capacity); \
        }                                                                                 \
        name->entries[name->count] = value;                                               \
        name->count++;                                                                    \
    }                                                                                     \
    void free_##T(T *name)                                                                \
    {                                                                                     \
        FREE_ARRAY(uint8_t, name->entries, name->capacity);                               \
        init_##T(name);                                                                   \
    }

#endif // _THOR_MEMORY_H_