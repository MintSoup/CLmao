#include <stdlib.h>
#include <string.h>

#include "mem.h"
#include "object.h"
#include "table.h"
#include "value.h"

void initTable(Table *t) {
	t->capacity = 0;
	t->count = 0;
	t->entries = NULL;
}

void freeTable(Table *t) {
	FREE_ARRAY(Entry, t->entries, t->capacity);
	initTable(t);
}

static Entry *findEntry(Entry *entries, int capacity, ObjString *key) {
	uint32_t index = key->hash % capacity;
	Entry *tombstone = NULL;
	while (true) {
		Entry *entry = &entries[index];
		if (entry->key == NULL) {
			if (IS_NULL(entry->value)) {
				return tombstone != NULL ? tombstone : entry;
			} else {
				if (tombstone == NULL)
					tombstone = entry;
			}
		} else if (entry->key == key) {
			return entry;
		}

		index = (index + 1) % capacity;
	}
}

static void adjustCapacity(Table *t, int capacity) {
	Entry *entries = ALLOCATE(Entry, capacity);
	for (int i = 0; i < capacity; i++) {
		entries[i].key = NULL;
		entries[i].value = NULL_VALUE;
	}
	t->count = 0;
	for (int i = 0; i < t->capacity; i++) {
		Entry *entry = &t->entries[i];
		if (entry->key == NULL)
			continue;
		Entry *dest = findEntry(entries, capacity, entry->key);
		if (!IS_NULL(dest->value)) {
			*dest = *entry;
			t->count++;
		}
	}
	FREE_ARRAY(Entry, t->entries, t->capacity);
	t->entries = entries;
	t->capacity = capacity;
}

bool tableSet(Table *t, ObjString *key, Value value) {
	if (t->count + 1 > t->capacity * TABLE_MAX_LOAD) {
		int capacity = GROW_CAPACITY(t->capacity);
		adjustCapacity(t, capacity);
	}

	Entry *entry = findEntry(t->entries, t->capacity, key);
	bool isNew = entry->key == NULL;
	if (isNew && IS_NULL(entry->value))
		t->count++;

	entry->key = key;
	entry->value = value;
	return isNew;
}
void tableAddAll(Table *src, Table *dest) {
	for (int i = 0; i < src->capacity; i++) {
		Entry *entry = &src->entries[i];
		if (entry->key != NULL) {
			tableSet(dest, entry->key, entry->value);
		}
	}
}

bool tableGet(Table *t, ObjString *key, Value *value) {
	if (t->count == 0)
		return false;

	Entry *e = findEntry(t->entries, t->capacity, key);
	if (e->key != NULL) {
		if (value != NULL)
			*value = e->value;
		return true;
	} else
		return false;
}

bool tableRemove(Table *t, ObjString *key) {
	int index = key->hash % t->capacity;
	Entry *entry = &t->entries[index];
	while (true) {
		if (entry->key == NULL)
			return false;
		if (entry->key == key) {
			entry->key = NULL;
			entry->value = BOOL_VALUE(true);
			return true;
		}
		index = (index + 1) % t->capacity;
	}
}
ObjString *findTableString(Table *t, const char *start, int length,
						   uint32_t hash) {

	if (t->count == 0)
		return NULL;
	uint32_t index = hash % t->capacity;
	for (;;) {
		Entry *entry = &t->entries[index];
		if (entry->key == NULL) {
			if (IS_NULL(entry->value))
				return NULL;
		} else if (entry->key->length == length && entry->key->hash == hash &&
				   memcmp(entry->key->chars, start, length) == 0) {
			return entry->key;
		}

		index = (index + 1) % t->capacity;
	}
}