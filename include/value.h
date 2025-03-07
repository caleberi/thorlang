#ifndef _THOR_VALUE_H_
#define _THOR_VALUE_H_
#include "memory.h"
#include "common.h"

INIT_ARRAY(value_array, Value);
GENERIC_ARRAY_OPS(value_array, Value);

void print_value(Value value);
bool values_equal(Value a, Value b);

#endif // _THOR_VALUE_H_