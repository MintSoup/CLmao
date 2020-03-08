#ifndef VALUE_H
#define VALUE_H

#include "commons.h"
#include "value.h"

typedef struct sObj Obj;
typedef struct sObjString ObjString;

typedef enum { VAL_NULL, VAL_BOOL, VAL_NUM, VAL_OBJ } ValueType;

typedef struct {
	ValueType type;
	union {
		bool boolean;
		double number;
		Obj *obj;
	} as;
} Value;

typedef struct {
	int count;
	int capacity;
	Value *values;
} ValueArray;

#define AS_BOOL(x) ((x).as.boolean)
#define AS_NUM(x) ((x).as.number)
#define AS_OBJ(x) ((x).as.obj)
#define AS_STRING(x) ((ObjString *)AS_OBJ(x))
#define AS_CSTRING(x) (AS_STRING(x)->chars)


#define IS_NULL(x) ((x).type == VAL_NULL)
#define IS_NUM(x) ((x).type == VAL_NUM)
#define IS_BOOL(x) ((x).type == VAL_BOOL)
#define IS_OBJ(x) ((x).type == VAL_OBJ)


#define BOOL_VALUE(x) ((Value){VAL_BOOL, {.boolean = x}})
#define NUM_VALUE(x) ((Value){VAL_NUM, {.number = x}})
#define OBJ_VALUE(x) ((Value){VAL_OBJ, {.obj = x}})
#define NULL_VALUE ((Value){VAL_NULL, {.number = 0}})

void initValueArray(ValueArray *array);
void writeValueArray(ValueArray *array, Value value);
void freeValueArray(ValueArray *array);
void printValue(Value value);
bool equal(Value a, Value b);

#endif