#ifndef _THOR_DEBUG_H_
#define _THOR_DEBUG_H_
#include "chunk.h"
#include <stdio.h>
void disassemble_chunk(Chunk *, const char *);
int disassemble_instruction(Chunk *, int);

#endif //_THOR_DEBUG_H_