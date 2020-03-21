#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "commons.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 256
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
	ObjClosure *closure;
	uint8_t *ip;
	Value *slots;
} Callframe;

typedef struct {
	Callframe frames[FRAMES_MAX];
	int frameCount;
	Value stack[STACK_MAX];
	Value *stackTop;
	Obj *objects;
	Table strings;
	Table globals;
	ObjUpvalue* openUpvalues;
	bool nativeError;
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