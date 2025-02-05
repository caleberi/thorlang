
#include "common.h"
#include "chunk.h"
#include "memory.h"

#define FREQ_SHIFT 0x10
#define LINE_MASK 0x7FFF // 0b111111111

GENERIC_ARRAY_IMPL(line_array, int, array, value);

void init_chunk(Chunk *chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    init_line_array(&chunk->lines);
    init_value_array(&chunk->constants);
}

// static void compress_line(Chunk *chunk, int line)
// {
//     if (chunk->count > 0)
//     {
//         int prev_line = chunk->lines.entries[chunk->lines.count - 1] & LINE_MASK;
//         int prev_freq = chunk->lines.entries[chunk->lines.count - 1] >> FREQ_SHIFT;

//         if (prev_line == line)
//         {
//             chunk->lines.entries[chunk->lines.count - 1] = ((prev_freq + 1) << FREQ_SHIFT) | (prev_line & LINE_MASK);
//             return;
//         }
//     }
//     chunk->lines.entries[chunk->lines.count] = (1 << FREQ_SHIFT) | line;
//     chunk->lines.count++;
// }

// int get_frequency(Chunk *chunk, int index)
// {
//     return chunk->lines.entries[index] >> FREQ_SHIFT;
// }

int get_line(Chunk *chunk, int index)
{
    return chunk->lines.entries[index];
}

void write_chunk(Chunk *chunk, uint8_t byte, int line)
{
    if (chunk->capacity < chunk->count + 1)
    {
        int old_cap = chunk->capacity;
        chunk->capacity = GROW_CAP(int, old_cap, RESIZE_PERCENT);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_cap, chunk->capacity);
    }
    write_line_array(&chunk->lines, line);
    chunk->code[chunk->count] = byte;
    chunk->count++;
}

void write_constant(Chunk *chunk, Value value, int line)
{
    int constant = add_constant(chunk, value);
    write_chunk(chunk, OP_CONSTANT_LONG, line);
    write_chunk(chunk, constant, line);
}

int add_constant(Chunk *chunk, Value value)
{
    write_value_array(&chunk->constants, value);
    return chunk->constants.count - 1;
}

void free_chunk(Chunk *chunk)
{
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    free_line_array(&chunk->lines);
    free_value_array(&chunk->constants);
    init_chunk(chunk);
}
