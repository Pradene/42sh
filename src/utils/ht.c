#include "ht.h"

#include <string.h>
#include <stdio.h>

// djb2 hash function
uint32_t djb2(const unsigned char *str) {
  uint32_t hash = 5381;

  while (*str) {
    hash = ((hash << 5) + hash) + *str++;
  }
  return hash;
}

static bool ht_resize(HashTable *ht, const size_t new_capacity) {
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
  ht->buckets = new_buckets;
  ht->capacity = new_capacity;

  return true;
}

void ht_insert(HashTable *ht, const char *key, void *value) {
  if (!key || !value) {
    return;
  }
  
  if (ht->capacity == 0) {
    ht_resize(ht, 32);
  }

  if (ht->size >= ht->capacity * 0.75) {
    ht_resize(ht, ht->capacity * 2);
  }

  uint32_t hash = djb2((unsigned char *)key) % ht->capacity;

  HtEntry *entry = ht->buckets[hash];
  while (entry) {
    if (strcmp(entry->key, key) == 0) {
      ht->free(entry->value);
      entry->value = value;

      return;
    }
    entry = entry->next;
  }

  entry = (HtEntry *)malloc(sizeof(HtEntry));
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

HtEntry *ht_get(const HashTable *ht, const char *key) {
  if (!ht || ht->capacity == 0) {
    return NULL;
  }

  if (!key) {
    return NULL;
  }

  uint32_t hash = djb2((const unsigned char *)key) % ht->capacity;
  HtEntry *entry = ht->buckets[hash];
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

      ht->free(node->value);
      free(node->key);
      free(node);

      ht->size--;
      return;
    }

    prev = node;
    node = node->next;
  }
}

void *ht_pop(HashTable *ht, const char *key) {
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

void ht_clear(HashTable *ht) {
  if (!ht) {
    return;
  }

  for (size_t i = 0; i < ht->capacity; ++i) {
    HtEntry *entry = ht->buckets[i];

    while (entry) {
      HtEntry *next = entry->next;
      ht->free(entry->value);
      free(entry->key);
      free(entry);
      entry = next;
    }

    ht->buckets[i] = NULL;
  }

  free(ht->buckets);
  ht->buckets = NULL;
  ht->size = 0;
  ht->capacity = 0;
}

void ht_destroy(HashTable *ht) {
  if (!ht) {
    return;
  }

  ht_clear(ht);
  free(ht);
}

HashTable *ht_with_capacity(const size_t capacity) {
  HashTable *ht = (HashTable *)malloc(sizeof(HashTable));
  if (!ht) {
    return NULL;
  }

  ht->buckets = NULL;
  ht->size = 0;
  ht->capacity = 0;
  if (!ht_resize(ht, capacity)) {
    free(ht);
    return NULL;
  }

  return ht;
}