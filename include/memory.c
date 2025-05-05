
#include "common.h"
#include "vm.h"

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

static void free_object(Obj *object)
{
    switch (object->type)
    {
    case OBJ_STRING:
    {
        ObjString *string = (ObjString *)object;
        FREE_ARRAY(char, string->chars, string->length + 1);
        FREE(ObjString, object);
        break;
    }
    }
}

void free_objects()
{
    Obj *object = vm.objects;
    while (object != NULL)
    {
        Obj *next = object->next;
        free_object(object);
        object = next;
    }
}
