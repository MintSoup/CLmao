#ifndef COMPILER_H
#define COMPILER_H


#include "scanner.h"
#include "commons.h"
#include "chunk.h"
#include "vm.h"

bool compile(const char* src ,Chunk* chunk);


#endif