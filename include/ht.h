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

  // Save delete function to correctly handle different types ?
} HashTable;

HashEntry *ht_get(const HashTable *ht, const char *key);
bool ht_contains(const HashTable* table, const char *key);
void ht_insert(HashTable *ht, const char *key, const void *value);
void ht_remove(HashTable *ht, const char *key);
void ht_clear(HashTable *ht);
