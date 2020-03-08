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