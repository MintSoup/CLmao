#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "value.h"
#include "table.h"
#include "object.h"
#include "commons.h"

#define FRAMES_MAX 256
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
	ObjFunction* func;
	uint8_t* ip;
	Value* slots;
} Callframe;

typedef struct {
	Callframe frames[FRAMES_MAX];
	int frameCount;
	Value stack[STACK_MAX];
	Value *stackTop;
	Obj *objects;
	Table strings;
	Table globals;
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

#endif