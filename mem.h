#ifndef MEM_H
#define MEM_H

#include "table.h"
#include "value.h"
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : ((capacity)*2))

void *reallocate(void *previous, size_t oldSize, size_t newSize);

#define GROW_ARRAY(previous, type, oldCount, count)                            \
	(type *)reallocate(previous, sizeof(type) * oldCount, sizeof(type) * count)

#define FREE_ARRAY(type, pointer, oldCount)                                    \
	reallocate(pointer, sizeof(type) * oldCount, 0);

#define ALLOCATE(type, size) ((type *)reallocate(NULL, 0, sizeof(type) * size))

#define FREE(type, pointer) (reallocate(pointer, sizeof(type), 0))

void markTable(Table* t);
void markObject(Obj* val);
void markValue(Value val);
void freeObjects();
void gc();
#endif