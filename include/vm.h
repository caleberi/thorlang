#ifndef _THOR_VM_H_
#define _THOR_VM_H_

#include "chunk.h"
#include "stack.h"
#include "table.h"

typedef enum InterpretResult
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

STACK(Value, stack);
typedef struct VM
{
    Chunk *chunk;
    uint8_t *ip;
    stack stack;
    Table strings;
    Table globals;

    Obj *objects;
} VM;

void init_vm();
void free_vm();
InterpretResult interpret(const char *source);
void push(Value value);
Value pop();

extern VM vm;

#endif //_THOR_VM_H_