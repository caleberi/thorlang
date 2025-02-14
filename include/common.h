#ifndef _THOR_COMMON_H_
#define _THOR_COMMON_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdarg.h>

#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION
typedef enum __attribute__((packed)) ValueType
{
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
} ValueType;

typedef struct Value
{
    ValueType type;
    union
    {
        bool boolean;
        double number;
    } pas;

} Value;

#define AS_BOOL(value) ((value).pas.boolean)
#define AS_NUMBER(value) ((value).pas.number)

#define BOOL_VAL(value) ((Value){VAL_BOOL, .pas = {.boolean = value}})
#define NIL_VAL ((Value){VAL_NIL, .pas = {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, .pas = {.number = value}})

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)

#endif // _THOR_COMMON_H_