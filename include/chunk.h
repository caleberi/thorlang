#ifndef _THOR_CHUNK_H_
#define _THOR_CHUNK_H_
#include "common.h"
#include "value.h"

INIT_ARRAY(line_array, int);
GENERIC_ARRAY_OPS(line_array, int);

// Represent operation with significant uint8 mapping
typedef enum Opcode
{
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    OP_RETURN,
} Opcode;

typedef struct Chunk
{
    int capacity;  // capacity of a dynamic chunk
    int count;     // current occupied space in the chunk
    uint8_t *code; // pointer to a specific opcode for VM
    value_array constants;
    line_array lines;
} Chunk;

/*@init_chunk - Creates a new chunk dynamic array of uint8_t codes*/
void init_chunk(Chunk *);
/*@free_chunk - Destroys a new chunk dynamic array of uint8_t codes*/
void free_chunk(Chunk *);
/*@write_chunk - Writes a new uint8_t code into last offset location*/
void write_chunk(Chunk *, uint8_t, int);
/*@write_constant -  Writes a long constant Value to the Chunk*/
void write_constant(Chunk *, Value, int);
/*@add_constant -  Writes a constant Value to the Chunk*/
int add_constant(Chunk *, Value);
/*@get_line - Retrieve the line the number of Chunk*/
int get_line(Chunk *chunk, int index);

#endif // _THOR_CHUNK_H_