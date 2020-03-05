#include <stdlib.h>

#include "chunk.h"
#include "mem.h"
#include "value.h"

void initChunk(Chunk *c) {
	c->count = 0;
	c->capacity = 0;
	c->code = NULL;
	c->lines = NULL;
	initValueArray(&(c->constants));
}

void writeChunk(Chunk *chunk, uint8_t byte, int line) {
	if (chunk->capacity < chunk->count + 1) {
		int oldCapacity = chunk->capacity;
		chunk->capacity = GROW_CAPACITY(oldCapacity);
		chunk->code =
			GROW_ARRAY(chunk->code, uint8_t, oldCapacity, chunk->capacity);
		chunk->lines =
			GROW_ARRAY(chunk->lines, int, oldCapacity, chunk->capacity);
	}
	chunk->code[chunk->count] = byte;
	chunk->lines[chunk->count] = line;
	(chunk->count)++;
}

void freeChunk(Chunk *chunk) {
	FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
	FREE_ARRAY(int, chunk->lines, chunk->capacity);

	freeValueArray(&(chunk->constants));
	initChunk(chunk);
}

int addConstant(Chunk *chunk, Value value) {
	writeValueArray(&(chunk->constants), value);
	return chunk->constants.count - 1;
}