#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef struct HtEntry {
  char           *key;
  void           *value;
  struct HtEntry *next;
} HtEntry;

typedef struct {
  HtEntry **buckets;
  size_t    size;
  size_t    capacity;
  void    (*free)(void *);
} HashTable;

static inline uint32_t djb2(const unsigned char *str) {
  uint32_t hash = 5381;
  while (*str)
    hash = ((hash << 5) + hash) + *str++;
  return hash;
}

static inline bool ht_resize(HashTable *ht, size_t new_capacity) {
  HtEntry **new_buckets = calloc(new_capacity, sizeof(HtEntry *));
  if (!new_buckets) {
    return false;
  }

  for (size_t i = 0; i < ht->capacity; ++i) {
    HtEntry *entry = ht->buckets[i];
    while (entry) {
      HtEntry *next = entry->next;
      uint32_t hash = djb2((unsigned char *)entry->key) % new_capacity;
      entry->next = new_buckets[hash];
      new_buckets[hash] = entry;
      entry = next;
    }
  }
  free(ht->buckets);
  ht->buckets  = new_buckets;
  ht->capacity = new_capacity;
  return true;
}

static inline HashTable *ht_with_capacity(size_t capacity) {
  HashTable *ht = malloc(sizeof(HashTable));
  if (!ht)
    return NULL;
  ht->buckets  = NULL;
  ht->size     = 0;
  ht->capacity = 0;
  ht->free     = NULL;
  if (!ht_resize(ht, capacity)) {
    free(ht);
    return NULL;
  }
  return ht;
}

static inline size_t ht_size(HashTable *ht) {
  return ht->size;
}

static inline bool ht_insert(HashTable *ht, const char *k, const void *v, size_t value_size) {
  if (!k || !v) {
    return false;
  }

  if (ht->size == 0 && !ht_resize(ht, 31)) {
    return false;
  }

  if (ht->size >= ht->capacity * 0.75 && !ht_resize(ht, ht->capacity * 2)) {
    return false;
  }

  uint32_t hash  = djb2((const unsigned char *)k) % ht->capacity;
  HtEntry *entry = ht->buckets[hash];

  while (entry) {
    if (strcmp(entry->key, k) == 0) {
      void *new_value = malloc(value_size);
      if (!new_value) {
        return false;
      }
      if (ht->free) {
        ht->free(entry->value);
      } else {
        free(entry->value);
      }
      entry->value = new_value;
      memcpy(entry->value, v, value_size);
      return true;
    }
    entry = entry->next;
  }

  entry = malloc(sizeof(HtEntry));
  if (!entry) {
    return false;
  }
  entry->key = strdup(k);
  if (!entry->key) {
    free(entry);
    return false;
  }
  entry->value = malloc(value_size);
  if (!entry->value) {
    free(entry->key);
    free(entry);
    return false;
  }
  memcpy(entry->value, v, value_size);
  entry->next = ht->buckets[hash];
  ht->buckets[hash] = entry;
  ht->size++;
  return true;
}

static inline HtEntry *ht_get(const HashTable *ht, const char *key) {
  if (!key || !ht || ht->capacity == 0) {
    return NULL;
  }
  uint32_t hash  = djb2((const unsigned char *)key) % ht->capacity;
  HtEntry *entry = ht->buckets[hash];
  while (entry) {
    if (strcmp(entry->key, key) == 0) {
      return entry;
    }
    entry = entry->next;
  }
  return NULL;
}

static inline bool ht_contains(const HashTable *ht, const char *key) {
  return ht_get(ht, key) != NULL;
}

static inline void ht_remove(HashTable *ht, const char *key) {
  if (!ht || ht->capacity == 0) {
    return;
  }

  uint32_t hash = djb2((const unsigned char *)key) % ht->capacity;
  HtEntry *node = ht->buckets[hash];
  HtEntry *prev = NULL;
  while (node) {
    if (strcmp(node->key, key) == 0) {
      if (prev) {
        prev->next = node->next;
      } else {
        ht->buckets[hash] = node->next;
      }
      if (ht->free) {
        ht->free(node->value);
      } else {
        free(node->value);
      }
      free(node->key);
      free(node);
      ht->size--;
      return;
    }
    prev = node;
    node = node->next;
  }
}

static inline void *ht_pop(HashTable *ht, const char *key) {
  if (!ht || ht->capacity == 0) {
    return NULL;
  }

  uint32_t hash = djb2((const unsigned char *)key) % ht->capacity;
  HtEntry *node = ht->buckets[hash];
  HtEntry *prev = NULL;
  while (node) {
    if (strcmp(node->key, key) == 0) {
      if (prev) {
        prev->next = node->next;
      } else {
        ht->buckets[hash] = node->next;
      }

      void *value = node->value;
      free(node->key);
      free(node);
      ht->size--;
      return value;
    }
    prev = node;
    node = node->next;
  }
  return NULL;
}

static inline void ht_clear(HashTable *ht) {
  if (!ht) {
    return;
  }

  for (size_t i = 0; i < ht->capacity; ++i) {
    HtEntry *entry = ht->buckets[i];
    while (entry) {
      HtEntry *next = entry->next;
      if (ht->free) {
        ht->free(entry->value);
      } else {
        free(entry->value);
      }
      free(entry->key);
      free(entry);
      entry = next;
    }
    ht->buckets[i] = NULL;
  }
  free(ht->buckets);
  ht->buckets  = NULL;
  ht->size     = 0;
  ht->capacity = 0;
}

static inline void ht_destroy(HashTable *ht) {
  if (!ht) {
    return;
  }

  ht_clear(ht);
  free(ht);
}
