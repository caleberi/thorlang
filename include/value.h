#ifndef _THOR_VALUE_H_
#define _THOR_VALUE_H_
#include "memory.h"
#include "common.h"

typedef enum __attribute__((packed)) ValueType
{
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,
} ValueType;

typedef struct Obj Obj;
typedef struct ObjString ObjString;
typedef struct Value
{
    ValueType type;
    union
    {
        bool boolean;
        double number;
        Obj *obj;
    } pas;
} Value;

#define AS_BOOL(value) ((value).pas.boolean)
#define AS_NUMBER(value) ((value).pas.number)
#define AS_OBJ(value) ((value).pas.obj)

#define BOOL_VAL(value) ((Value){VAL_BOOL, .pas = {.boolean = value}})
#define NIL_VAL ((Value){VAL_NIL, .pas = {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, .pas = {.number = value}})
#define OBJ_VAL(object) ((Value){.pas = {.obj = (Obj *)object}, .type = VAL_OBJ})

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

INIT_ARRAY(value_array, Value);
GENERIC_ARRAY_OPS(value_array, Value);

void print_value(Value value);
bool values_equal(Value a, Value b);

#endif // _THOR_VALUE_H_