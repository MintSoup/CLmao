#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
	Chunk *chunk;
	uint8_t *ip;
	Value stack[STACK_MAX];
	Value *stackTop;
	Obj *objects;
} VM;

extern VM vm;

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
void freeVM();

InterpretResult interpret(const char *src);

void push(Value val);
Value pop();
static void resetStack();
static Value peek(int distance);
static void runtimeError(const char *format, ...);

#endif