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
int simpleInstruction(char *name, int offset) {
	printf("%s\n", name);
	return offset + 1;
}
static int byteInstruction(char *name, Chunk *chunk, int offset) {
	uint8_t level = chunk->code[offset + 1];
	printf("%-16s %4u\n", name, level);
	return offset + 2;
}
static int shortInstruction(char *name, Chunk *chunk, int offset) {
	uint16_t insts = chunk->code[offset + 2] | (chunk->code[offset + 1] << 8);
	printf("%-16s %4u\n", name, insts);
	return offset + 3;
}

static int constantInstruction(char *name, Chunk *chunk, int offset) {
	uint8_t constant = chunk->code[offset + 1];
	printf("%-16s %4u '", name, constant);
	printValue(chunk->constants.values[constant]);
	printf("'\n");
	return offset + 2;
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
	case OP_NEGATE: {
		return simpleInstruction("OP_NEGATE", offset);
	}
	case OP_ADD: {
		return simpleInstruction("OP_BIN_ADD", offset);
	}
	case OP_SUB: {
		return simpleInstruction("OP_BIN_SUB", offset);
	}
	case OP_MUL: {
		return simpleInstruction("OP_BIN_MUL", offset);
	}
	case OP_DIV: {
		return simpleInstruction("OP_BIN_DIV", offset);
	}
	case OP_FALSE: {
		return simpleInstruction("OP_FALSE", offset);
	}
	case OP_TRUE: {
		return simpleInstruction("OP_TRUE", offset);
	}
	case OP_NULL: {
		return simpleInstruction("OP_NULL", offset);
	}
	case OP_NOT: {
		return simpleInstruction("OP_NOT", offset);
	}
	case OP_PRINT: {
		return simpleInstruction("OP_PRINT", offset);
	}
	case OP_EQUALS: {
		return simpleInstruction("OP_EQUALS", offset);
		break;
	}
	case OP_NOT_EQUALS: {
		return simpleInstruction("OP_NOT_EQUALS", offset);
		break;
	}
	case OP_GREATER: {
		return simpleInstruction("OP_GREATER", offset);
		break;
	}
	case OP_LESS: {
		return simpleInstruction("OP_LESS", offset);
		break;
	}
	case OP_GREATER_EQUAL: {
		return simpleInstruction("OP_GREATER_EQUAL", offset);
		break;
	}
	case OP_LESS_EQUAL: {
		return simpleInstruction("OP_LESS_EQUAL", offset);
		break;
	}
	case OP_FACTORIAL: {
		return simpleInstruction("OP_FACTORIAL", offset);
		break;
	}
	case OP_POP: {
		return simpleInstruction("OP_POP", offset);
	}
	case OP_DEFINE_GLOBAL: {
		return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
	}
	case OP_GET_GLOBAL: {
		return constantInstruction("OP_GET_GLOBAL", chunk, offset);
	}
	case OP_SET_GLOBAL: {
		return constantInstruction("OP_SET_GLOBAL", chunk, offset);
	}
	case OP_GET_LOCAL: {
		return byteInstruction("OP_GET_LOCAL", chunk, offset);
	}
	case OP_SET_LOCAL: {
		return byteInstruction("OP_SET_LOCAL", chunk, offset);
	}
	case OP_POPN: {
		return byteInstruction("OP_POPN", chunk, offset);
	}
	case OP_JUMP_IF_FALSE: {
		return shortInstruction("OP_JUMP_IF_FALSE", chunk, offset);
	}
	case OP_JUMP: {
		return shortInstruction("OP_JUMP", chunk, offset);
	}
	case OP_LOOP: {
		return shortInstruction("OP_LOOP", chunk, offset);
	}

	default: {
		printf("unknown upcode: 0x%x\n", instruction);
		return offset + 1;
	}
	}
}
