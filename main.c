#include "commons.h"
#include <stdio.h>

#include "chunk.h"
#include "dbg.h"

int main() {
	Chunk c;
	initChunk(&c);
	writeChunk(&c, OP_RETURN, 13);
	writeChunk(&c, OP_CONSTANT, 13);
	writeChunk(&c, addConstant(&c, 512.3), 13);

	disassembleChunk(&c, "test chunk");
	freeChunk(&c);
	return 0;
}