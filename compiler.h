#ifndef COMPILER_H
#define COMPILER_H

#include "scanner.h"
#include "commons.h"
#include "chunk.h"
#include "vm.h"

ObjFunction *compile(const char *src);
void markCompilerRoots();

#endif