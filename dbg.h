#ifndef DBG_H
#define DBG_H

#include "chunk.h"

void disassembleChunk(Chunk *chunk, char *name);
int disassembleInstruction(Chunk *chunk, int offset);
int simpleInstruction(char *name, int offset);
#endif