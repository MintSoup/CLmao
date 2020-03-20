#include <stdlib.h>

#include "commons.h"
#include "mem.h"
#include "object.h"
#include "vm.h"

void *reallocate(void *previous, size_t oldSize, size_t newSize) {
	if (newSize == 0) {
		free(previous);
		return NULL;
	}
	return realloc(previous, newSize);
}

static void freeObject(Obj *b) {
	switch (b->type) {
	case OBJ_STRING: {
		ObjString *str = (ObjString *)b;
		FREE_ARRAY(char, str->chars, str->length + 1);
		FREE(ObjString, str);
		break;
	}
	case OBJ_FUNCTION: {
		ObjFunction *f = (ObjFunction *)b;
		freeChunk(&(f->chunk));
		FREE(ObjFunction, f);
		break;
	}
	case OBJ_NATIVE: {
		FREE(ObjNative, (ObjNative *)b);
		break;
	}
	case OBJ_CLOSURE: {
		ObjClosure *c = (ObjClosure *)b;
		FREE_ARRAY(ObjUpvalue *, c->upvalues, c->upvalueCount);
		FREE(ObjClosure, (ObjClosure *)b);
		break;
	}
	case OBJ_UPV: {
		FREE(ObjUpvalue, (ObjUpvalue *)b);
		break;
	}
	}
}

void freeObjects() {
	Obj *object = vm.objects;
	while (object != NULL) {
		Obj *next = object->next;
		freeObject(object);
		object = next;
	}
}