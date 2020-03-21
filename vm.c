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
#include <time.h>
VM vm;

static void resetStack();
static Value peek(int distance);
static void runtimeError(const char *format, ...);
static bool callValue(Value callee, int args);
static void defineNative(const char *name, NativeFn function);

static Value clockNative(int argCount, Value *args) {
	if (argCount != 0) {
		runtimeError("Builtin clock() function takes no arguments.");
		vm.nativeError = true;
	}
	return NUM_VALUE((double)clock());
}

static Value slenNative(int argCount, Value *args) {
	if (argCount != 1 || !IS_STRING(*args)) {
		runtimeError("Builtin slen function takes 1 string argument.");
		vm.nativeError = true;
	}
	return NUM_VALUE((double)AS_STRING(*args)->length);
}
static Value strNative(int argCount, Value *args) {
	if (argCount != 1) {
		runtimeError("Builtin slen function takes 1.");
		vm.nativeError = true;
	}
	ObjString *r;
	switch (args->type) {
	case VAL_BOOL: {
		r = copyString(AS_BOOL(*args) ? "true" : "false",
					   AS_BOOL(*args) ? 4 : 5);
		break;
	}
	case VAL_NULL: {
		r = copyString("null", 4);
		break;
	}
	case VAL_NUM: {
		if (IS_INT(*args)) {
			char otp[15];
			int len = sprintf(otp, "%d", (int)round(AS_NUM(*args)));
			r = copyString(otp, len);
			break;
		}
		char otp[20];
		int len = sprintf(otp, "%g", AS_NUM(*args));
		r = copyString(otp, len);
		break;
	}
	default: {
		runtimeError("Cannot stringify objects.");
		vm.nativeError = true;
		r = copyString("", 0);
	}
	}
	return OBJ_VALUE((Obj *)r);
}

void initVM() {
	resetStack();
	vm.objects = NULL;
	initTable(&vm.strings);
	initTable(&vm.globals);
	vm.nativeError = false;

	defineNative("clock", clockNative);
	defineNative("slen", slenNative);
	defineNative("str", strNative);
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
		return AS_NUM(v) != 0;
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

static ObjUpvalue *captureUpvalue(Value *local) {

	ObjUpvalue *prevUpvalue = NULL;
	ObjUpvalue *upvalue = vm.openUpvalues;

	while (upvalue != NULL && upvalue->location > local) {
		prevUpvalue = upvalue;
		upvalue = upvalue->next;
	}
	if (upvalue != NULL && upvalue->location == local)
		return upvalue;
	ObjUpvalue *createdUpvalue = newUpvalue(local);
	createdUpvalue->next = upvalue;
	if (prevUpvalue == NULL) {
		vm.openUpvalues = createdUpvalue;
	} else {
		prevUpvalue->next = createdUpvalue;
	}

	return createdUpvalue;
}

static void closeUpvalues(Value *last) {
	while (vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
		ObjUpvalue *upvalue = vm.openUpvalues;
		upvalue->closed = *upvalue->location;
		upvalue->location = &upvalue->closed;
		vm.openUpvalues = upvalue->next;
	}
}

static InterpretResult run() {
	Callframe *frame = &(vm.frames[vm.frameCount - 1]);

#define READ_BYTE() (*frame->ip++)
#define READ_CONSTANT()                                                        \
	(frame->closure->func->chunk.constants.values[READ_BYTE()])
#define READ_STRING() (AS_STRING(READ_CONSTANT()))
#define READ_SHORT()                                                           \
	(frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
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

		disassembleInstruction(
			&frame->closure->func->chunk,
			(int)(frame->ip - frame->closure->func->chunk.code));
#endif

		switch (instruction) {
		case OP_RETURN: {
			Value result = pop();
			closeUpvalues(frame->slots);
			vm.frameCount--;
			if (vm.frameCount == 0) {
				pop();
				return INTERPRET_OK;
			}
			vm.stackTop = frame->slots;
			push(result);
			frame = &vm.frames[vm.frameCount - 1];
			break;
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
		case OP_SET_LOCAL: {
			uint8_t level = READ_BYTE();
			frame->slots[level] = peek(0);
			break;
		}
		case OP_GET_LOCAL: {
			uint8_t level = READ_BYTE();
			push(frame->slots[level]);
			break;
		}
		case OP_POPN: {
			uint8_t count = READ_BYTE();
			vm.stackTop -= count;
			break;
		}
		case OP_JUMP_IF_FALSE: {
			uint16_t offset = READ_SHORT();
			if (!isTruthy(peek(0)))
				frame->ip += offset;
			break;
		}
		case OP_JUMP: {
			uint16_t offset = READ_SHORT();
			frame->ip += offset;
			break;
		}
		case OP_LOOP: {
			uint16_t offset = READ_SHORT();
			frame->ip -= offset;
			break;
		}
		case OP_MODULO: {
			Value a = peek(0);
			Value b = peek(1);
			if (IS_INT(a) && (round(AS_NUM(a)) >= 1)) {
				if (IS_INT(b) && (round(AS_NUM(b)) >= 0)) {
					int right = round(AS_NUM(pop()));
					int left = round(AS_NUM(pop()));
					push(NUM_VALUE((double)(left % right)));
				} else {
					runtimeError("Modulo supported only on positive ints.");
					return INTERPRET_RUNTIME_ERROR;
				}
			} else {
				runtimeError("Modulo supported only on positive ints.");
				return INTERPRET_RUNTIME_ERROR;
			}

			break;
		}
		case OP_CALL: {
			int args = READ_BYTE();
			if (!callValue(peek(args), args)) {
				return INTERPRET_RUNTIME_ERROR;
			}
			frame = &vm.frames[vm.frameCount - 1];
			break;
		}
		case OP_CLOSURE: {
			ObjFunction *func = AS_FUNCTION(READ_CONSTANT());
			ObjClosure *closure = newClosure(func);
			push(OBJ_VALUE((Obj *)closure));
			for (int i = 0; i < closure->upvalueCount; i++) {
				uint8_t isLocal = READ_BYTE();
				uint8_t index = READ_BYTE();
				if (isLocal) {
					closure->upvalues[i] = captureUpvalue(frame->slots + index);
				} else {
					closure->upvalues[i] = frame->closure->upvalues[index];
				}
			}
			break;
		}
		case OP_SET_UPV: {
			uint8_t slot = READ_BYTE();
			*frame->closure->upvalues[slot]->location = peek(0);
			break;
		}
		case OP_GET_UPV: {
			uint8_t slot = READ_BYTE();
			push(*frame->closure->upvalues[slot]->location);
			break;
		}
		case OP_CLOSE_UPV:
			closeUpvalues(vm.stackTop - 1);
			pop();
			break;
		case OP_MAP:
			if (!(IS_INT(peek(0)) && round(AS_NUM(peek(0))) >= 0)) {
				runtimeError("Map index can only be positive integer.");
				return INTERPRET_RUNTIME_ERROR;
			}
			int index = (int)round(AS_NUM(pop()));
			if (!IS_STRING(peek(0))) {
				runtimeError("Only strings are maps.");
				return INTERPRET_RUNTIME_ERROR;
			}
			ObjString *str = (ObjString *)AS_OBJ(pop());
			if (index >= str->length) {
				runtimeError("Map index is too large. (%d / %d).", index,
							 str->length);
				return INTERPRET_RUNTIME_ERROR;
			}
			ObjString *nstr = copyString(str->chars + index, 1);
			push(OBJ_VALUE((Obj *)nstr));
			break;
		default: {
			runtimeError("Cringe unknown instruction");
			return INTERPRET_RUNTIME_ERROR;
		}
		}
#undef READ_CONSTANT
#undef READ_BYTE
#undef READ_STRING
#undef READ_SHORT
	}
}

InterpretResult interpret(const char *src) {
#ifdef DEBUG_CLOCKS

	clock_t start;
	clock_t end;
	start = clock();

#endif

	ObjFunction *script = compile(src);

#ifdef DEBUG_CLOCKS
	end = clock();
	printf("\nCompiling took %ld ms.\n", end - start);
#endif

	if (script == NULL)
		return INTERPRET_COMPILE_ERROR;

	push(OBJ_VALUE((Obj *)script));
	ObjClosure *closure = newClosure(script);
	pop();
	push(OBJ_VALUE((Obj *)closure));
	callValue(OBJ_VALUE((Obj *)closure), 0);

#ifdef DEBUG_CLOCKS
	start = clock();
#endif
	InterpretResult res = run();
#ifdef DEBUG_CLOCKS
	end = clock();
	printf("\nRunning took %ld ms.\n", end - start);
#endif

	return res;
}

static void resetStack() {
	vm.stackTop = vm.stack;
	vm.frameCount = 0;
	vm.openUpvalues = NULL;
}
static void runtimeError(const char *format, ...) {
	va_list args;
	va_start(args, format);
	for (int i = vm.frameCount - 1; i >= 0; i--) {
		Callframe *frame = &vm.frames[i];
		ObjFunction *function = frame->closure->func;

		size_t instruction = frame->ip - function->chunk.code - 1;
		fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
		if (function->name == NULL) {
			fprintf(stderr, "<script>\n");
		} else {
			fprintf(stderr, "%s()\n", function->name->chars);
		}
	}
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);
	resetStack();
}
static void defineNative(const char *name, NativeFn function) {
	push(OBJ_VALUE((Obj *)copyString(name, (int)strlen(name))));
	push(OBJ_VALUE((Obj *)newNative(function)));
	tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
	pop();
	pop();
}
void push(Value val) { *(vm.stackTop++) = val; }
Value pop() { return *(--vm.stackTop); }

static Value peek(int distance) { return vm.stackTop[-1 - distance]; }
static bool call(ObjClosure *closure, int args) {
	if (args != closure->func->arity) {
		runtimeError("Expected %d arguments in function call, but got %d.",
					 closure->func->arity, args);
		return false;
	}
	if (vm.frameCount == FRAMES_MAX)
		runtimeError("Stack overflow.");
	Callframe *frame = &vm.frames[vm.frameCount++];
	frame->closure = closure;
	frame->ip = closure->func->chunk.code;

	frame->slots = vm.stackTop - args - 1;
	return true;
}

static bool callValue(Value callee, int args) {
	if (IS_OBJ(callee)) {
		switch (AS_OBJ(callee)->type) {
		case OBJ_CLOSURE: {
			return call(AS_CLOSURE(callee), args);
		}
		case OBJ_NATIVE: {
			NativeFn native = AS_NATIVE(callee)->f;
			Value result = native(args, vm.stackTop - args);
			if (vm.nativeError)
				return false;
			vm.stackTop -= args + 1;
			push(result);
			return true;
		}

		default:
			break;
		}
	}

	runtimeError("Only classes and functions are callable");
	return false;
}
