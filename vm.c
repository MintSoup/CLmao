#include "vm.h"
#include "commons.h"
#include "dbg.h"
#include <stdio.h>

VM vm;

void initVM() { resetStack(); }

void freeVM() {}

static InterpretResult run() {
#define READ_BYTE() (*(vm.ip++))
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OPERATOR(o) (push(pop() o pop()))

	while (true) {
		uint8_t instruction = READ_BYTE();
#ifdef DEBUG_TRACE_EXECUTION
		printf("          ");
		for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
			printf("[ ");
			printValue(*slot);
			printf(" ]");
		}
		printf("\n");

		disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif

		switch (instruction) {
		case OP_RETURN: {
			return INTERPRET_OK;
		}
		case OP_CONSTANT: {
			Value constant = READ_CONSTANT();
			push(constant);
			break;
		}
		case OP_NEGATE: {
			push(-pop());
			break;
		}
		case OP_BIN_ADD: {
			BINARY_OPERATOR(+);
			break;
		}
		case OP_BIN_SUB: {
			BINARY_OPERATOR(-);
			break;
		}
		case OP_BIN_MUL: {
			BINARY_OPERATOR(*);
			break;
		}
		case OP_BIN_DIV: {
			BINARY_OPERATOR(/);
			break;
		}
		case OP_PRINT: {
			printf("%g", pop());
			break;
		}

		default: {}
		}
#undef READ_CONSTANT
#undef READ_BYTE
	}
}

InterpretResult interpret(Chunk *chunk) {
	vm.chunk = chunk;
	vm.ip = vm.chunk->code;
	return run();
}

static void resetStack() { vm.stackTop = vm.stack; }

void push(Value val) { *(vm.stackTop++) = val; }
Value pop() { return *(--vm.stackTop); }