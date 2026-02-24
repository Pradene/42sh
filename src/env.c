#include "env.h"
#include "vec.h"
#include "utils.h"
#include "ht.h"

#include <stdio.h>

StatusCode env_from_cstr_array(HashTable *env, const char **envp) {
  for (size_t i = 0; envp[i]; ++i) {
    char *equal = strchr(envp[i], '=');
    if (!equal) {
      continue;
    } else {
      size_t key_length = equal - envp[i];
      char *key = strndup(envp[i], key_length);
      char *value = strdup(equal + 1);
      
      if (strcmp(value, "") == 0) {
        free(key);
        free(value);
        continue;
      }

      ht_insert(env, key, value);
      
      free(key);
      free(value);
    }
  }

  return OK;
}

char **env_to_cstr_array(const HashTable *env) {
  char **array = malloc((env->size + 1) * sizeof(char *));
  if (!array) {
    return NULL;
  }

  size_t index = 0;
  for (size_t i = 0; i < env->capacity; ++i) {
    HashEntry *node = env->buckets[i];
    while (node) {
      size_t len = strlen(node->key) + strlen((char *)node->value) + 2;
      array[index] = malloc(len);
      snprintf(array[index], len, "%s=%s", node->key, (char *)node->value);
      index++;
      node = node->next;
    }
  }

  array[index] = NULL;

  return array;
}

void env_free(HashTable *env) {
  ht_clear(env);
}

char *env_find(const HashTable *env, const char *name) {
  HashEntry *entry = ht_get(env, name);
  return entry ? (char *)entry->value : NULL;
}

StatusCode env_set(HashTable *env, const char *name, const char *value) {
  HashEntry *entry = ht_get(env, name);
  if (entry) {
    char *new = strdup(value);
    if (!new) {
      return MEM_ALLOCATION_FAILED;
    }

    free(entry->value);
    entry->value = new;
  } else {
    ht_insert(env, name, (void *)value);
  }
  return OK;
}

StatusCode env_unset(HashTable *env, const char *name) {
  ht_remove(env, name);
  return OK;
}

void env_print(const HashTable *env) {  
  for (size_t i = 0; i < env->capacity; ++i) {
    HashEntry *entry = env->buckets[i];
    while (entry) {
      printf("%s=%s\n", entry->key, (char *)entry->value);
      entry = entry->next;
    }
  }
}
