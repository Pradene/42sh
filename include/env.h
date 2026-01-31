#pragma once

#include <stdlib.h>

#include "status.h"

typedef struct {
  char **data;
  size_t size;
  size_t capacity;
} Environment;

void env_free(Environment *env);
StatusCode env_copy(Environment *env, const char **envp);
char *env_find(const Environment *env, const char *name);
StatusCode env_set(Environment *env, const char *name, const char *value);
void env_print(const Environment *env);
