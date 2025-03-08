#include "object.h"
#include "value.h"
#include "memory.h"

GENERIC_ARRAY_IMPL(value_array, Value, array, value);

void print_value(Value value)
{
    switch (value.type)
    {
    case VAL_BOOL:
        printf(AS_BOOL(value) ? "true" : "false");
        break;
    case VAL_NIL:
        printf("nil");
        break;
    case VAL_NUMBER:
        printf("%g", AS_NUMBER(value));
        break;
    case VAL_OBJ:
        print_object(value);
        break;
    }
}

bool values_equal(Value a, Value b)
{
    if (a.type != b.type)
        return false;
    switch (a.type)
    {
    case VAL_BOOL:
        return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:
        return true;
    case VAL_NUMBER:
        return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ:
        ObjString *aString = AS_STRING(a);
        ObjString *bString = AS_STRING(b);
        return aString->length == bString->length &&
               memcmp(aString->chars, bString->chars, aString->length) == 0;
    default:
        return false;
    }
}