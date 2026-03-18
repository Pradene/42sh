#include "ht.h"
#include "42sh.h"
#include "utils.h"
#include "vec.h"

#include <stdio.h>

void hash_insert(HashTable *ht, const char *key, const char *path) {
  CacheEntry entry = (CacheEntry){
    .path = strdup(path),
    .hits = 0,
  };
  ht_insert(ht, key, &entry);
}

CacheEntry *hash_get(HashTable *ht, const char *key) {
  HtEntry *e = ht_get(ht, key);
  return e ? (CacheEntry *)e->value : NULL;
}

void hash_entry_free(void *value) {
  CacheEntry *e = (CacheEntry *)value;
  free(e->path);
  free(e);
}

void hash_clear(HashTable *ht) {
  ht_clear(ht);
}

void hash_print(HashTable *ht) {
  if (ht_size(ht) == 0) {
    printf("hash: hash table empty\n");

  } else {
    printf("hits\tcommand\n");
    for (size_t i = 0; i < ht->capacity; ++i) {
      HtEntry *entry = ht->buckets[i];
      while (entry) {
        CacheEntry *e = (CacheEntry *)entry->value;
        printf("%4zu\t%s\n", e->hits, e->path);
        entry = entry->next;
      }
    }
  }
}

void builtin_hash(AstNode *node) {
  size_t argc = vec_size(&node->command.args);
  char **args = node->command.args.data;
  size_t opt_end = 1;

  if (argc == 1) {
    hash_print(hash);
    return;
  }

  if (!strcmp(args[1], "-r")) {
    hash_clear(hash);
    ++opt_end;
  }

  for (size_t i = opt_end; i < argc; ++i) {
    char *path = find_command_path(args[i]);
    if (!path) {
      fprintf(stderr, "hash: %s: not found\n", args[i]);
      exit_status = 1;
    } else {
      hash_insert(hash, args[i], path);
      free(path);
    }
  }
}
