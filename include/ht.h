#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct HtEntry {
  char             *key;
  void             *value;
  struct HtEntry *next;
} HtEntry;

typedef struct {
  HtEntry **buckets;
  size_t      size;
  size_t      capacity;
  
  void        (*free)(void *);
} HashTable;

HashTable *ht_with_capacity(const size_t capacity);
void       ht_insert(HashTable *ht, const char *key, void *value);
bool       ht_contains(const HashTable* table, const char *key);
HtEntry   *ht_get(const HashTable *ht, const char *key);
void       ht_remove(HashTable *ht, const char *key);
void      *ht_pop(HashTable *ht, const char *key);
void       ht_clear(HashTable *ht);
void       ht_destroy(HashTable *ht);
