#include "dbg.h"
#include "chunk.h"
#include "value.h"
#include <stdio.h>

void disassembleChunk(Chunk *chunk, char *name) {
	printf("=====%s====\n", name);

	for (int o = 0; o < chunk->count;) {
		o = disassembleInstruction(chunk, o);
	}
}
int disassembleInstruction(Chunk *chunk, int offset) {
	printf("%04d ", offset);

	if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
		printf("   | ");
	} else {
		printf("%4d ", chunk->lines[offset]);
	}
	
	uint8_t instruction = chunk->code[offset];
	switch (instruction) {
	case OP_RETURN: {
		return simpleInstruction("OP_RETURN", offset);
	}
	case OP_CONSTANT: {
		return constantInstruction("OP_CONSTANT", chunk, offset);
	}

	default: {
		printf("unknown upcode: 0x%x\n", instruction);
		return offset + 1;
	}
	}
}

int simpleInstruction(char *name, int offset) {
	printf("%s\n", name);
	return offset + 1;
}
static int constantInstruction(char *name, Chunk *chunk, int offset) {
	uint8_t constant = chunk->code[offset + 1];
	printf("%-16s %4d '", name, constant);
	printValue(chunk->constants.values[constant]);
	printf("'\n");
	return offset + 2;
}