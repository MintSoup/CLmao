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
	OP_POPN,
	OP_JUMP_IF_FALSE,
	OP_JUMP,
	OP_CALL,
	OP_LOOP,
	OP_MODULO,
	OP_CLOSURE,
	OP_SET_UPV,
	OP_GET_UPV,
	OP_CLOSE_UPV,
	OP_MAP,
	OP_CLASS,
	OP_GET_FIELD,
	OP_SET_FIELD,
	OP_METHOD,
	OP_INVOKE
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