
#include "common.h"

void *reallocate(void *pointer, size_t old_cap, size_t new_cap)
{
    if (new_cap == 0)
    {
        free(pointer);
        return NULL;
    }
    void *result = realloc(pointer, new_cap);
    if (result == NULL)
        exit(1);
    return result;
}
