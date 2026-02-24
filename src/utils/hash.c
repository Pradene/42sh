#include "ht.h"

#include <string.h>
#include <stdio.h>

// djb2 hash function
uint64_t djb2(const unsigned char *str) {
  uint64_t hash = 5381;

  while (*str)
    hash = ((hash << 5) + hash) + *str++;
  return hash;
}

static bool ht_resize(HashTable *ht, size_t new_capacity) {
  HashEntry **new_buckets = calloc(new_capacity, sizeof(HashEntry *));
  if (!new_buckets) {
    return false;
  }

  for (size_t i = 0; i < ht->capacity; ++i) {
    HashEntry *entry = ht->buckets[i];

    while (entry) {
      HashEntry *next = entry->next;
      uint64_t index = djb2((unsigned char *)entry->key) % new_capacity;
      entry->next = new_buckets[index];
      new_buckets[index] = entry;
      entry = next;
    }
  }

  free(ht->buckets);
  ht->buckets = new_buckets;
  ht->capacity = new_capacity;

  return true;
}

void ht_insert(HashTable *ht, const char *key, void *value) {
  if (ht->capacity == 0) {
    ht_resize(ht, 32);
  }

  if (ht->size >= ht->capacity * 0.75) {
    ht_resize(ht, ht->capacity * 2);
  }

  uint64_t hash = djb2((unsigned char *)key) % ht->capacity;

  HashEntry *entry = ht->buckets[hash];
  while (entry) {
    if (strcmp(entry->key, key) == 0) {
      free(entry->value);
      entry->value = value;
      return;
    }
    entry = entry->next;
  }

  entry = (HashEntry *)malloc(sizeof(HashEntry));
  if (!entry) {
    return;
  }
  entry->key = strdup(key);
  entry->value = value;
  entry->next = ht->buckets[hash];
  ht->buckets[hash] = entry;

  ht->size++;
}

bool ht_contains(const HashTable* ht, const char *key) {
  return ht_get(ht, key) != NULL;
}

HashEntry *ht_get(const HashTable *ht, const char *key) {
  if (!ht || ht->capacity == 0) {
    return NULL;
  }

  uint32_t hash = djb2((const unsigned char *)key) % ht->capacity;
  HashEntry *entry = ht->buckets[hash];
  while (entry) {
    if (strcmp(entry->key, key) == 0) {
      return entry;
    }
    entry = entry->next;
  }

  return NULL;
}

void ht_remove(HashTable *ht, const char *key) {
  if (!ht || ht->capacity == 0) {
    return;
  }

  uint64_t hash = djb2((const unsigned char *)key) % ht->capacity;

  HashEntry *node = ht->buckets[hash];
  HashEntry *prev = NULL;

  while (node) {
    if (strcmp(node->key, key) == 0) {
      if (prev) {
        prev->next = node->next;
      } else {
        ht->buckets[hash] = node->next;
      }

      free(node->key);
      ht->free(node->value);
      free(node);

      ht->size--;
      return;
    }

    prev = node;
    node = node->next;
  }
}

void ht_clear(HashTable *ht) {
  for (size_t i = 0; i < ht->capacity; ++i) {
    HashEntry *entry = ht->buckets[i];

    while (entry) {
      HashEntry *next = entry->next;
      free(entry->key);
      ht->free(entry->value);
      free(entry);
      entry = next;
    }

    ht->buckets[i] = NULL;
  }

  ht->size = 0;
  free(ht->buckets);
}