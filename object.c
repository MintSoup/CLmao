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

void printObject(Value val) {
	switch (OBJ_TYPE(val)) {
	case OBJ_STRING:
		printf("%s", AS_CSTRING(val));
		break;
	default:
		break;
	}
}