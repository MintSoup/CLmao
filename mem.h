#ifndef MEM_H
#define MEM_H

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : ((capacity)*2))

void *reallocate(void *previous, size_t oldSize, size_t newSize);

#define GROW_ARRAY(previous, type, oldCount, count)                            \
	(type *)reallocate(previous, sizeof(type) * oldCount, sizeof(type) * count)

#define FREE_ARRAY(type, pointer, oldCount)                                    \
	reallocate(pointer, sizeof(type) * oldCount, 0);

#endif