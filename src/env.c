#include "env.h"
#include "vec.h"
#include "utils.h"
#include "ht.h"
#include "42sh.h"

#include <stdio.h>


void env_variable_free(void *value) {
  Variable *variable = (Variable *)value;
  free(variable->content);
  free(variable);
}

StatusCode env_from_cstr_array(HashTable *env, const char **envp) {
  for (size_t i = 0; envp[i]; ++i) {
    char *equal = strchr(envp[i], '=');
    if (!equal) {
      continue;
    } else {
      size_t key_length = equal - envp[i];
      char *key = strndup(envp[i], key_length);
      char *content = strdup(equal + 1);
      if (strcmp(content, "") == 0) {
        free(key);
        free(content);
        continue;
      }
      
      Variable *value = (Variable *)malloc(sizeof(Variable));
      value->content = content;
      value->exported = true;
      value->readonly = false;
      ht_insert(env, key, value);
      
      free(key);
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
      size_t len = strlen(node->key) + strlen((char *)((Variable *)(node->value))->content) + 2;
      array[index] = malloc(len);
      snprintf(array[index], len, "%s=%s", node->key, (char *)((Variable *)(node->value))->content);
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
  return entry ? (char *)((Variable *)(entry->value))->content : NULL;
}

StatusCode env_set(HashTable *env, const char *name, void *value) {
  HashEntry *entry = ht_get(env, name);
  if (entry) {
    env->free(entry->value);
    entry->value = value;
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
      printf("%s=%s\n", entry->key, (char *)((Variable *)(entry->value))->content);
      entry = entry->next;
    }
  }
}
