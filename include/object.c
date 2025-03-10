#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type *)allocate_object(sizeof(type), objectType)

#define ALLOCATE_OBJ_PLUS_CHARS(type, objectType, length) \
    (type *)allocate_object((sizeof(type) + (length + 1)), objectType)

static Obj *
allocate_object(size_t size, ObjType type)
{
    Obj *object = (Obj *)reallocate(NULL, 0, size);
    object->type = type;

    object->next = vm.objects;
    vm.objects = object;
    return object;
}

ObjString *copy_string(const char *chars, int length)
{
    char *heap_chars = ALLOCATE(char, length + 1);
    memcpy(heap_chars, chars, length);
    heap_chars[length] = '\0';
    return allocate_string(heap_chars, length);
}

ObjString *take_string(char *chars, int length)
{
    return allocate_string(chars, length);
}

static ObjString *allocate_string(char *chars, int length)
{
    ObjString *string = ALLOCATE_OBJ_PLUS_CHARS(ObjString, OBJ_STRING, length);
    if (string == NULL)
        return NULL;
    string->length = length;
    memcpy(string->chars, chars, length);
    string->chars[length] = '\0';
    return string;
}

void print_object(Value value)
{
    switch (OBJ_TYPE(value))
    {
    case OBJ_STRING:
        printf("%s", AS_CSTRING(value));
        break;
    }
}