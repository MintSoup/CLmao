#ifndef OBJECT_H
#define OBJECT_H

#include "commons.h"
#include "value.h"

#define OBJ_TYPE(x) (AS_OBJ(x)->type)

#define IS_STRING(x) (isObjType(x, OBJ_STRING))

typedef enum { OBJ_STRING } ObjType;

struct sObj {
	ObjType type;
	struct sObj *next;
};

struct sObjString {
	Obj obj;
	int length;
	char *chars;
	uint32_t hash;
};

ObjString *copyString(const char *start, size_t length);

static inline bool isObjType(Value x, ObjType type) {
	return IS_OBJ(x) && AS_OBJ(x)->type == type;
}
ObjString *copyString(const char *start, size_t length);
ObjString *takeString(const char *start, size_t length);

void printObject(Value val);

#endif