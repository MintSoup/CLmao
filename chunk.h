#ifndef CHUNK_H
#define CHUNK_H

#include "commons.h"
#include "value.h"
typedef enum {
	OP_RETURN,
	OP_CONSTANT,
	OP_NEGATE,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_PRINT,
	OP_TRUE,
	OP_FALSE,
	OP_NULL,
	OP_NOT,
	OP_EQUALS,
	OP_NOT_EQUALS,
	OP_LESS,
	OP_LESS_EQUAL,
	OP_GREATER,
	OP_GREATER_EQUAL,
	OP_FACTORIAL,
	OP_POP,
	OP_DEFINE_GLOBAL,
	OP_GET_GLOBAL,
	OP_SET_GLOBAL,
	OP_GET_LOCAL,
	OP_SET_LOCAL,
	OP_POPN
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