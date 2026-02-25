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
    HashEntry *entry = env->buckets[i];
    while (entry) {
      Variable *v = (Variable *)entry->value;
      if (v && v->exported) {
        size_t length = strlen(entry->key) + strlen((char *)v->content) + 2;
        array[index] = malloc(sizeof(char) * length);
        snprintf(array[index], length, "%s=%s", entry->key, (char *)v->content);
        ++index;
      }
      entry = entry->next;
    }
  }

  array[index] = NULL;

  return array;
}

char *env_find(const HashTable *env, const char *name) {
  HashEntry *entry = ht_get(env, name);
  return entry ? (char *)((Variable *)(entry->value))->content : NULL;
}

StatusCode env_unset(HashTable *env, const char *name) {
  ht_remove(env, name);
  return OK;
}

void env_print(const HashTable *env) {  
  for (size_t i = 0; i < env->capacity; ++i) {
    HashEntry *entry = env->buckets[i];
    while (entry) {
      Variable *v = (Variable *)entry->value;
      if (v && v->exported) {
        printf("%s=%s\n", entry->key, v->content);
      }
      entry = entry->next;
    }
  }
}
