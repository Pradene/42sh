#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct HashEntry {
  char *key;
  void *value;
  struct HashEntry *next;
} HashEntry;

typedef struct {
  HashEntry **buckets;
  size_t size;
  size_t capacity;
  
  size_t value_size;
  void (*free)(void *);
} HashTable;

void ht_insert(HashTable *ht, char *key, void *value);
bool ht_contains(const HashTable* table, const char *key);
HashEntry *ht_get(const HashTable *ht, const char *key);
void ht_remove(HashTable *ht, const char *key);
void ht_clear(HashTable *ht);
