#include "mem.h"
#include "commons.h"
#include "compiler.h"
#include "object.h"
#include "vm.h"
#include <stdlib.h>

#ifdef DEBUG_LOGGC
#include "dbg.h"
#include <stdio.h>
#endif
void *reallocate(void *previous, size_t oldSize, size_t newSize) {
	vm.bytesAllocated += newSize - oldSize;

	if (newSize > oldSize) {
#ifdef DEBUG_STRESSGC
		gc();
#endif
		if (vm.bytesAllocated > vm.nextGC) {
			gc();
		}
	}

	if (newSize == 0) {
		free(previous);
		return NULL;
	}
	return realloc(previous, newSize);
}

static void freeObject(Obj *b) {

#ifdef DEBUG_LOGGC
	printf("freed %d at %p\n", b->type, (void *)b);
#endif
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
		FREE(ObjClosure, c);
		break;
	}
	case OBJ_UPV: {
		FREE(ObjUpvalue, (ObjUpvalue *)b);
		break;
	}
	case OBJ_CLASS: {
		ObjClass *c = (ObjClass *)b;
		freeTable(&c->methods);
		FREE(ObjClass, c);
		break;
	}
	case OBJ_INSTANCE: {
		ObjInstance *instance = (ObjInstance *)b;
		freeTable(&instance->fields);
		FREE(ObjInstance, instance);
		break;
	}
	case OBJ_METHOD: {
		FREE(ObjMethod, b);
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

void markObject(Obj *obj) {
	if (obj == NULL || obj->isMarked)
		return;
	obj->isMarked = true;

	if (vm.greyCapacity < vm.greyCount + 1) {
		vm.greyCapacity = GROW_CAPACITY(vm.greyCapacity);
		vm.greyStack = realloc(vm.greyStack, vm.greyCapacity * sizeof(Obj *));
	}

	vm.greyStack[vm.greyCount++] = obj;

#ifdef DEBUG_LOGGC
	printf("marked %p ", (void *)obj);
	printValue(OBJ_VALUE(obj));
	puts("");
#endif
}

void markValue(Value val) {
	if (!IS_OBJ(val))
		return;
	markObject(AS_OBJ(val));
}

void markTable(Table *t) {
	for (int i = 0; i < t->capacity; i++) {
		markObject((Obj *)(t->entries[i].key));
		markValue(t->entries[i].value);
	}
}

static void markRoots() {
	for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
		markValue(*slot);
	}
	markTable(&vm.globals);
	for (int i = 0; i < vm.frameCount; i++) {
		markObject((Obj *)vm.frames[i].closure);
	}

	for (ObjUpvalue *upv = vm.openUpvalues; upv != NULL; upv = upv->next) {
		markObject((Obj *)upv);
	}

	markCompilerRoots();
}

static void markArray(ValueArray *arr) {
	for (int i = 0; i < arr->count; i++) {
		markValue(arr->values[i]);
	}
}

static void blackenObject(Obj *obj) {
	switch (obj->type) {
	case OBJ_NATIVE:
	case OBJ_STRING:
		break;
	case OBJ_UPV: {
		ObjUpvalue *upv = (ObjUpvalue *)obj;
		markValue(upv->closed);
		break;
	}
	case OBJ_FUNCTION: {
		ObjFunction *func = (ObjFunction *)obj;
		markObject((Obj *)func->name);
		markArray(&func->chunk.constants);
		break;
	}
	case OBJ_CLOSURE: {
		ObjClosure *closure = (ObjClosure *)obj;
		markObject((Obj *)closure->func);
		for (int i = 0; i < closure->upvalueCount; i++) {
			markObject((Obj *)closure->upvalues[i]);
		}
		break;
	}
	case OBJ_CLASS: {
		ObjClass *klass = (ObjClass *)obj;
		markObject((Obj *)klass->name);
		markTable(&klass->methods);
		break;
	}
	case OBJ_INSTANCE: {
		ObjInstance *instance = (ObjInstance *)obj;
		markObject((Obj *)instance->klass);
		markTable(&instance->fields);
		break;
	}
	case OBJ_METHOD: {
		ObjMethod *m = (ObjMethod *)obj;
		markObject((Obj *)m->closure);
		markValue(m->parent);
		break;
	}
	}

#ifdef DEBUG_LOGGC
	printf("blackened %p ", (void *)obj);
	printObject(OBJ_VALUE(obj));
	puts("");
#endif
}

static void traceReferences() {
	while (vm.greyCount) {
		blackenObject((vm.greyStack[--vm.greyCount]));
	}
}

static void sweep() {
	Obj *current = vm.objects;
	Obj *previous = NULL;
	while (current != NULL) {
		if (current->isMarked) {
			current->isMarked = false;
			previous = current;
			current = current->next;
		} else {
			Obj *unreached = current;

			current = current->next;
			if (previous != NULL) {
				previous->next = current;
			} else {
				vm.objects = current;
			}
			freeObject(unreached);
		}
	}
}

void gc() {

#ifdef DEBUG_LOGGC
	printf("-------Garbage Collector--------\n");
	size_t before = vm.bytesAllocated;
#endif

	markRoots();
	traceReferences();
	tableRemoveWhite(&vm.strings);
	sweep();

	vm.nextGC = vm.bytesAllocated * 5;

#ifdef DEBUG_LOGGC

	printf("   collected %I64u bytes (from %I64u to %I64u) next at %I64u\n",
		   before - vm.bytesAllocated, before, vm.bytesAllocated, vm.nextGC);

	printf("-------Garbage Collector end--------\n");
#endif
}