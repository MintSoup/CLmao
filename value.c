#include "value.h"
#include "commons.h"
#include "mem.h"
#include <stdio.h>
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
	}
}
