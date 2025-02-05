#include "value.h"

GENERIC_ARRAY_IMPL(value_array, Value, array, value);

void print_value(Value value)
{
    printf("%g", value);
}