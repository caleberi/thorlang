#ifndef _THOR_STACK_H_
#define _THOR_STACK_H_
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>

#define PERCENT_GROWTH 10

#define STACK(T, name)              \
    typedef struct Stack            \
    {                               \
        T *top;                     \
        T *storage;                 \
        int size;                   \
        int capacity;               \
    } name;                         \
    void init_##name(name *s, int); \
    int is_empty_##name(name *s);   \
    int size_##name(name *s);       \
    void push_##name(name *s, T);   \
    T pop_##name(name *s);          \
    T *top_##name(name *s);         \
    void free_##name(name *s);      \
    void reset_##name(name *s)

#define STACK_IMPL(T, VT, stack)                                                                           \
    void init_##T(stack *s, int capacity)                                                                  \
    {                                                                                                      \
        s->size = 0;                                                                                       \
        s->capacity = GROW_CAP(int, capacity, PERCENT_GROWTH);                                             \
        s->storage = GROW_ARRAY(VT, s->storage, 0, s->capacity);                                           \
        if (s->storage == NULL)                                                                            \
        {                                                                                                  \
            fprintf(stderr, "Insufficient memory to initialize stack [(%s) (%d)].\n", __FILE__, __LINE__); \
            exit(1);                                                                                       \
        }                                                                                                  \
        s->top = s->storage;                                                                               \
    }                                                                                                      \
    int is_empty_##T(stack *s) { return (s->size == 0); }                                                  \
    int size_##T(stack *s) { return s->size; }                                                             \
    void push_##T(stack *s, VT elem)                                                                       \
    {                                                                                                      \
        if (s->size >= s->capacity)                                                                        \
        {                                                                                                  \
            int old_cap = s->capacity;                                                                     \
            s->capacity = GROW_CAP(int, old_cap, PERCENT_GROWTH);                                          \
            s->storage = GROW_ARRAY(VT, s->storage, old_cap, s->capacity);                                 \
            s->top = s->storage + s->size;                                                                 \
        }                                                                                                  \
        *s->top = elem;                                                                                    \
        s->top++;                                                                                          \
        s->size++;                                                                                         \
    }                                                                                                      \
    VT *top_##T(stack *s) { return s->top - 1; }                                                           \
    VT pop_##T(stack *s)                                                                                   \
    {                                                                                                      \
        if (is_empty_##T(s))                                                                               \
        {                                                                                                  \
            fprintf(stderr, "Can not pop from an empty stack. [(%s) (%d)].\n", __FILE__, __LINE__);        \
            exit(1);                                                                                       \
        }                                                                                                  \
        s->top--;                                                                                          \
        s->size--;                                                                                         \
        return *s->top;                                                                                    \
    }                                                                                                      \
    void free_##T(stack *s)                                                                                \
    {                                                                                                      \
        free(s->storage);                                                                                  \
        s->top = NULL;                                                                                     \
        s->size = 0;                                                                                       \
        s->capacity = 0;                                                                                   \
    }                                                                                                      \
    void reset_##T(stack *s)                                                                               \
    {                                                                                                      \
        s->top = s->storage;                                                                               \
        s->size = 0;                                                                                       \
    }

#endif // _THOR_STACK_H_