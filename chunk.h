#ifndef CHUNK_H
#define CHUNK_H

#include "commons.h"
#include "value.h"
typedef enum {
	OP_RETURN,
	OP_CONSTANT,
	OP_NEGATE,
	OP_BIN_ADD,
	OP_BIN_SUB,
	OP_BIN_MUL,
	OP_BIN_DIV,
	OP_PRINT
} OpCode;

typedef struct {
	int count;
	int capacity;
	uint8_t *code;
	ValueArray constants;
	int *lines;
} Chunk;

void initChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte, int line);
void freeChunk(Chunk *chunk);
int addConstant(Chunk *chunk, Value value);

#endif