#ifndef _THOR_COMPILER_H_
#define _THOR_COMPILER_H_
#include "chunk.h"
#include "scanner.h"

typedef struct
{
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
} Parser;

bool compile(const char *source, Chunk *chunk);
void boostrap_keyword();
void cleanup_keyword();

#endif // _THOR_COMPILE_H_
