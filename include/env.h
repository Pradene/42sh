#pragma once

#include <stdlib.h>

#include "status.h"
#include "ht.h"

typedef struct {
  char *content;
  bool readonly;
  bool exported;
} Variable;

void env_variable_free(void *value);
void env_free(HashTable *env);
StatusCode env_from_cstr_array(HashTable *env, const char **envp);
char **env_to_cstr_array(const HashTable *env);
char *env_find(const HashTable *env, const char *name);
StatusCode env_set(HashTable *env, const char *name, void *value);
StatusCode env_unset(HashTable *env, const char *name);
void env_print(const HashTable *env);
