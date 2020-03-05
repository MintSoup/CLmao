#include "commons.h"
#include <stdio.h>

#include "chunk.h"
#include "dbg.h"
#include "vm.h"

int main() {
	Chunk c;
	initChunk(&c);

	initVM();
	writeChunk(&c, OP_CONSTANT, 14);
	writeChunk(&c, addConstant(&c, -10), 14);
	writeChunk(&c, OP_CONSTANT, 13);
	writeChunk(&c, addConstant(&c, 5), 13);
	writeChunk(&c, OP_CONSTANT, 13);
	writeChunk(&c, addConstant(&c, 4), 13);
	writeChunk(&c, OP_BIN_MUL, 13);
	writeChunk(&c, OP_NEGATE, 14);

	writeChunk(&c, OP_BIN_DIV, 14);
	writeChunk(&c, OP_PRINT, 14);
	disassembleChunk(&c, "test chunk");

	puts("----------------------------------------------------------");

	interpret(&c);

	freeChunk(&c);
	freeVM();

	return 0;
}