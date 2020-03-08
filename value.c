#include "value.h"
#include "commons.h"
#include "mem.h"
#include "object.h"
#include <stdio.h>
#include <string.h>
void initValueArray(ValueArray *array) {
	array->capacity = 0;
	array->count = 0;
	array->values = NULL;
}
void writeValueArray(ValueArray *array, Value value) {
	if (array->capacity < array->count + 1) {
		int oldCapacity = array->capacity;
		array->capacity = GROW_CAPACITY(oldCapacity);
		array->values =
			GROW_ARRAY(array->values, Value, oldCapacity, array->capacity);
	}
	array->values[array->count] = value;
	(array->count)++;
}
void freeValueArray(ValueArray *array) {
	FREE_ARRAY(Value, array->values, array->capacity);
	initValueArray(array);
}
void printValue(Value value) {
	switch (value.type) {
	case VAL_NUM:
		printf("%g", AS_NUM(value));
		break;
	case VAL_BOOL:
		printf(AS_BOOL(value) ? "true" : "false");
		break;
	case VAL_NULL:
		printf("NULL");
		break;
	case VAL_OBJ:
		printObject(value);
		break;
	}
}

bool equal(Value a, Value b) {
	if (a.type != b.type)
		return false;
	switch (a.type) {
	case VAL_BOOL:
		return AS_BOOL(a) == AS_BOOL(b);
	case VAL_NULL:
		return true;
	case VAL_NUM:
		return AS_NUM(a) == AS_NUM(b);
	case VAL_OBJ:
		return strcmp(AS_CSTRING(a), AS_CSTRING(b)) == 0;
	}
}
