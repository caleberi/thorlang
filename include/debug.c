#include "debug.h"

static int simple_instruction(const char *name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

static int constant_instruction(const char *name, Chunk *chunk,
                                int offset)
{
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    print_value(chunk->constants.entries[constant]);
    printf("'\n");
    return offset + 2;
}

void disassemble_chunk(Chunk *chunk, const char *name)
{
    printf("== %s ==\n", name);
    for (int offset = 0; offset < chunk->count;)
        offset = disassemble_instruction(chunk, offset);
}

int disassemble_instruction(Chunk *chunk, int offset)
{
    printf("%04d ", offset);

    if (offset > 0 && get_line(chunk, offset) == get_line(chunk, offset - 1))
        printf("   | ");
    else
        printf("%4d ", get_line(chunk, offset));

    uint8_t instruction = chunk->code[offset];
    switch (instruction)
    {
    case OP_RETURN:
        return simple_instruction("OP_RETURN", offset);
    case OP_NIL:
        return simpleInstruction("OP_NIL", offset);
    case OP_TRUE:
        return simpleInstruction("OP_TRUE", offset);
    case OP_FALSE:
        return simple_instruction("OP_FALSE", offset);
    case OP_EQUAL:
        return simple_instruction("OP_EQUAL", offset);
    case OP_GREATER:
        return simple_instruction("OP_GREATER", offset);
    case OP_LESS:
        return simple_instruction("OP_LESS", offset);
    case OP_ADD:
        return simple_instruction("OP_ADD", offset);
    case OP_SUBTRACT:
        return simple_instruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
        return simple_instruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:
        return simple_instruction("OP_DIVIDE", offset);
    case OP_NOT:
        return simple_instruction("OP_NOT", offset);
    case OP_NEGATE:
        return simple_instruction("OP_NEGATE", offset);
    case OP_CONSTANT:
        return constant_instruction("OP_CONSTANT", chunk, offset);
    case OP_CONSTANT_LONG:
        return constant_instruction("OP_CONSTANT_LONG", chunk, offset);
    case OP_PRINT:
        return simple_instruction("OP_PRINT", offset);
    case OP_POP:
        return simple_instruction("OP_POP", offset);
    case OP_DEFINE_GLOBAL:
        return constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}
