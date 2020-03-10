#include "chunk.h"
#include "commons.h"
#include "dbg.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>

void runFile(char *name);
char *readFile(char *name);

int main(int argc, char *argv[]) {
	initVM();
#ifndef DEBUG_BUILD

	if (argc == 1) {
		printf("Usage: lmao <filename>\n");
		return 1;
	} else if (argc == 2) {
		runFile(argv[1]);
	}

#else
	runFile("test.lmao");
#endif
	return 0;
}

char *readFile(char *name) {
	FILE *f = fopen(name, "rb");
	if (f == NULL) {
		fprintf(stderr, "Cannot open file: %s\n", name);
		exit(1);
	}
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	rewind(f);
	char *buf = (char *)malloc(size + 1);
	if (buf == NULL) {
		fprintf(stderr, "Not enough memory to read file\n", name);
		exit(1);
	}
	size_t bytesRead = fread(buf, sizeof(char), size, f);
	if (bytesRead < size) {
		fprintf(stderr, "File read failed\n");
	}
	fclose(f);
	buf[size] = 0;
	return buf;
}

void runFile(char *name) {
	char *src = readFile(name);

	
	InterpretResult i = interpret(src);
	free(src);

	if (i == INTERPRET_OK) {

	} else if (i == INTERPRET_COMPILE_ERROR) {
		exit(69);
	} else if (i == INTERPRET_RUNTIME_ERROR) {
		exit(420);
	}

}