#include "object.h"
#include "mem.h"
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

static ObjString *allocateString(const char *start, size_t length) {
	ObjString *str = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	str->chars = (char *)start;
	str->length = length;
	return str;
}

ObjString *takeString(const char *start, size_t length) {
	return allocateString(start, length);
}

ObjString *copyString(const char *start, size_t length) {
	char *heapChars = ALLOCATE(char, length + 1);
	memcpy(heapChars, start, length);
	heapChars[length] = 0;

	return allocateString(heapChars, length);
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