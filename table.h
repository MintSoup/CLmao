#ifndef TABLE_H
#define TABLE_H

#include "commons.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

typedef struct {
	ObjString *key;
	Value value;
} Entry;

typedef struct {
	int count;
	int capacity;
	Entry *entries;
} Table;

void initTable(Table *t);
void freeTable(Table *t);
bool tableSet(Table *t, ObjString *key, Value value);
bool tableSet(Table *t, ObjString *key, Value value);
void tableAddAll(Table *src, Table *dest);
bool tableGet(Table *t, ObjString *key, Value *value);
bool tableRemove(Table *t, ObjString *key);
ObjString *findTableString(Table *t, const char *start, int length,
						   uint32_t hash);
#endif