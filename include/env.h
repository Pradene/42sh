#pragma once

#include <stdlib.h>

#include "status.h"
#include "ht.h"

void env_free(HashTable *env);
StatusCode env_from_cstr_array(HashTable *env, const char **envp);
char **env_to_cstr_array(const HashTable *env);
char *env_find(const HashTable *env, const char *name);
StatusCode env_set(HashTable *env, const char *name, const char *value);
StatusCode env_unset(HashTable *env, const char *name);
void env_print(const HashTable *env);
