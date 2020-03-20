#include "object.h"
#include "commons.h"
#include "mem.h"
#include "table.h"
#include "vm.h"
#include <stdio.h>
#include <string.h>
#define ALLOCATE_OBJ(type, otype) ((type *)allocateObject(sizeof(type), otype))

static Obj *allocateObject(size_t size, ObjType type) {
	Obj *object = (Obj *)reallocate(NULL, 0, size);
	object->type = type;
	object->next = vm.objects;
	vm.objects = object;
	return object;
}

static ObjString *allocateString(const char *start, size_t length,
								 uint32_t hash) {
	ObjString *str = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	str->chars = (char *)start;
	str->length = length;
	str->hash = hash;
	tableSet(&vm.strings, str, NULL_VALUE);

	return str;
}

static uint32_t hashString(const char *key, int length) {
	uint32_t hash = 2166136261u;

	for (int i = 0; i < length; i++) {
		hash ^= key[i];
		hash *= 16777619;
	}

	return hash;
}

ObjString *takeString(const char *start, size_t length) {
	uint32_t hash = hashString(start, length);
	ObjString *interned = findTableString(&vm.strings, start, length, hash);
	if (interned != NULL) {
		FREE_ARRAY(char, (char *)start, length + 1);
		return interned;
	}
	return allocateString(start, length, hash);
}

ObjString *copyString(const char *start, size_t length) {
	uint32_t hash = hashString(start, length);
	ObjString *interned = findTableString(&vm.strings, start, length, hash);
	if (interned != NULL)
		return interned;
	char *heapChars = ALLOCATE(char, length + 1);
	memcpy(heapChars, start, length);
	heapChars[length] = 0;

	return allocateString(heapChars, length, hash);
}

static void printFunction(ObjFunction *x) {
	if (x->name == NULL) {
		printf("<script>");
		return;
	}
	printf("<fn %s>", x->name->chars);
}

void printObject(Value val) {
	switch (OBJ_TYPE(val)) {
	case OBJ_STRING:
		printf("%s", AS_CSTRING(val));
		break;
	case OBJ_FUNCTION: {
		printFunction(AS_FUNCTION(val));
		break;
	}
	case OBJ_NATIVE: {
		printf("<native>");
		break;
	}
	case OBJ_CLOSURE: {
		printFunction(AS_CLOSURE(val)->func);
		break;
	}
	case OBJ_UPV: {
		puts("upvalue");
		break;
	}
	default:
		break;
	}
}

ObjFunction *newFunction(NativeFn func) {
	ObjFunction *e = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
	e->arity = 0;
	e->name = NULL;
	e->upvalueCount = 0;
	initChunk(&(e->chunk));
	return e;
}
ObjNative *newNative(NativeFn func) {
	ObjNative *nat = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
	nat->f = func;
	return nat;
}
ObjClosure *newClosure(ObjFunction *func) {
	ObjClosure *cls = (ObjClosure *)ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
	cls->func = func;

	ObjUpvalue **upvalues = ALLOCATE_OBJ(ObjUpvalue *, OBJ_UPV);
	for (int i = 0; i < func->upvalueCount; i++) {
		upvalues[i] = NULL;
	}
	cls->upvalues = upvalues;
	cls->upvalueCount = func->upvalueCount;
	return cls;
}
ObjUpvalue *newUpvalue(Value *slot) {
	ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPV);
	upvalue->location = slot;
	return upvalue;
}