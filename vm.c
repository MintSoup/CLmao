#include "vm.h"
#include "commons.h"
#include "compiler.h"
#include "dbg.h"
#include "mem.h"
#include "object.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

VM vm;

static void resetStack();
static Value peek(int distance);
static void runtimeError(const char *format, ...);


void initVM() {
	resetStack();
	vm.objects = NULL;
	initTable(&vm.strings);
	initTable(&vm.globals);
}

void freeVM() {
	freeObjects();
	freeTable(&vm.strings);
	freeTable(&vm.globals);
}

static bool isTruthy(Value v) {
	switch (v.type) {
	case VAL_BOOL:
		return AS_BOOL(v);
	case VAL_NUM:
		return true;
	case VAL_NULL:
		return false;
	case VAL_OBJ:
		return true;
	}
	return false;
}

static void concat() {
	ObjString *str2 = AS_STRING(pop());
	ObjString *str1 = AS_STRING(pop());

	size_t len = str1->length + str2->length;
	char *newStr = ALLOCATE(char, len + 1);

	memcpy(newStr, str1->chars, str1->length);
	memcpy(newStr + str1->length, str2->chars, str2->length + 1);

	ObjString *new = takeString(newStr, len);
	push(OBJ_VALUE((Obj *)new));
}

static InterpretResult run() {

#define READ_BYTE() (*(vm.ip++))
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_STRING() (AS_STRING(READ_CONSTANT()))
#define BINARY_OPERATOR(o, valueType)                                          \
	do {                                                                       \
		if (!(IS_NUM(peek(0)) && IS_NUM(peek(1)))) {                           \
			runtimeError("Operation not supported on those types");            \
			return INTERPRET_RUNTIME_ERROR;                                    \
		}                                                                      \
		double b = AS_NUM(pop());                                              \
		double a = AS_NUM(pop());                                              \
		push(valueType(a o b));                                                \
	} while (false)

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
			if (!IS_NUM(peek(0))) {
				runtimeError("Operand must be a number.");
				return INTERPRET_RUNTIME_ERROR;
			}
			push(NUM_VALUE(-AS_NUM(pop())));
			break;
		}
		case OP_ADD: {
			if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
				concat();
			} else {
				BINARY_OPERATOR(+, NUM_VALUE);
			}

			break;
		}
		case OP_SUB: {
			BINARY_OPERATOR(-, NUM_VALUE);
			break;
		}
		case OP_MUL: {
			BINARY_OPERATOR(*, NUM_VALUE);
			break;
		}
		case OP_DIV: {
			BINARY_OPERATOR(/, NUM_VALUE);
			break;
		}
		case OP_NULL: {
			push(NULL_VALUE);
			break;
		}
		case OP_TRUE: {
			push(BOOL_VALUE(true));
			break;
		}
		case OP_FALSE: {
			push(BOOL_VALUE(false));
			break;
		}
		case OP_NOT: {
			push(BOOL_VALUE(!isTruthy(pop())));
			break;
		}
		case OP_EQUALS: {
			push(BOOL_VALUE(equal(pop(), pop())));
			break;
		}
		case OP_NOT_EQUALS: {
			push(BOOL_VALUE(!equal(pop(), pop())));
			break;
		}
		case OP_GREATER: {
			BINARY_OPERATOR(>, BOOL_VALUE);
			break;
		}
		case OP_LESS: {
			BINARY_OPERATOR(<, BOOL_VALUE);
			break;
		}
		case OP_GREATER_EQUAL: {
			BINARY_OPERATOR(>=, BOOL_VALUE);
			break;
		}
		case OP_LESS_EQUAL: {
			BINARY_OPERATOR(<=, BOOL_VALUE);
			break;
		}
		case OP_FACTORIAL: {
			Value v = peek(0);
			if (IS_INT(v) && (round(AS_NUM(v)) >= 0)) {
				double value = AS_NUM(pop());
				int output = 1;
				for (int i = 2; i <= value; i++) {
					output *= i;
				}
				push(NUM_VALUE((double)output));
			} else {
				runtimeError("Factorial can only be used on positive integers");
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
		}
		case OP_PRINT: {
			printValue(pop());
			puts("");
			break;
		}
		case OP_POP: {
			pop();
			break;
		}
		case OP_DEFINE_GLOBAL: {
			ObjString *name = READ_STRING();
			tableSet(&vm.globals, name, peek(0));
			pop();
			break;
		}
		case OP_GET_GLOBAL: {
			ObjString *name = READ_STRING();
			Value value;
			if (!tableGet(&vm.globals, name, &value)) {
				runtimeError("Variable %s not defined", name->chars);
				return INTERPRET_RUNTIME_ERROR;
			}
			push(value);
			break;
		}
		case OP_SET_GLOBAL: {
			ObjString *name = READ_STRING();
			Value set = peek(0);
			if (tableSet(&vm.globals, name, set)) {
				tableRemove(&vm.globals, name);
				runtimeError("Variable %s not defined", name->chars);
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
		}

		default: {}
		}
#undef READ_CONSTANT
#undef READ_BYTE
#undef READ_STRING
	}
}

InterpretResult interpret(const char *src) {
	Chunk chunk;
	initChunk(&chunk);

	if (!compile(src, &chunk)) {
		freeChunk(&chunk);
		return INTERPRET_COMPILE_ERROR;
	}
	vm.chunk = &chunk;
	vm.ip = vm.chunk->code;

	InterpretResult res = run();
	freeChunk(&chunk);

	return res;
}

static void resetStack() { vm.stackTop = vm.stack; }
static void runtimeError(const char *format, ...) {
	va_list args;
	va_start(args, format);
	size_t instruction = vm.ip - vm.chunk->code;
	int line = vm.chunk->lines[instruction];
	fprintf(stderr, "[line %d] ", line);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);
	resetStack();
}

void push(Value val) { *(vm.stackTop++) = val; }
Value pop() { return *(--vm.stackTop); }

static Value peek(int distance) { return vm.stackTop[-1 - distance]; }
