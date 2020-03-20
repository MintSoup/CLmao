#ifndef OBJECT_H
#define OBJECT_H

#include "chunk.h"
#include "commons.h"
#include "value.h"

#define OBJ_TYPE(x) (AS_OBJ(x)->type)

#define AS_STRING(x) ((ObjString *)AS_OBJ(x))
#define AS_CSTRING(x) (AS_STRING(x)->chars)
#define AS_FUNCTION(x) ((ObjFunction *)AS_OBJ(x))
#define AS_NATIVE(x) ((ObjNative *)AS_OBJ(x))
#define AS_CLOSURE(x) ((ObjClosure *)AS_OBJ(x))
#define AS_UPV(x) ((ObjUpvalue *)AS_OBJ(x))

#define IS_STRING(x) (isObjType(x, OBJ_STRING))
#define IS_FUNCTION(x) (isObjType(x, OBJ_FUNCTION))
#define IS_NATIVE(x) (isObjType(x, OBJ_NATIVE))
#define IS_CLOSURE(x) (isObjType(x, OBJ_CLOSURE))
#define IS_UPV(x) (isObjType(x, OBJ_UPV))

typedef enum {
	OBJ_STRING,
	OBJ_FUNCTION,
	OBJ_NATIVE,
	OBJ_CLOSURE,
	OBJ_UPV
} ObjType;

struct sObj {
	ObjType type;
	struct sObj *next;
};

typedef struct {
	Obj obj;
	int arity;
	Chunk chunk;
	ObjString *name;
	int upvalueCount;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value *args);

typedef struct {
	Obj obj;
	NativeFn f;
} ObjNative;

struct sObjString {
	Obj obj;
	int length;
	char *chars;
	uint32_t hash;
};

typedef struct sUpvalue {
	Obj obj;
	Value *location;
} ObjUpvalue;


typedef struct {
	Obj obj;
	ObjFunction *func;
	ObjUpvalue** upvalues;
	int upvalueCount;
} ObjClosure;


ObjString *copyString(const char *start, size_t length);

static inline bool isObjType(Value x, ObjType type) {
	return IS_OBJ(x) && AS_OBJ(x)->type == type;
}
ObjString *copyString(const char *start, size_t length);
ObjString *takeString(const char *start, size_t length);

void printObject(Value val);

ObjFunction *newFunction();
ObjNative *newNative(NativeFn func);
ObjClosure *newClosure(ObjFunction *func);
ObjUpvalue *newUpvalue(Value *slot);

#endif