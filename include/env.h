#pragma once

#include <stdlib.h>

#include "status.h"

typedef struct {
  char **data;
  size_t size;
  size_t capacity;
} Environment;

void env_free(Environment *env);
StatusCode env_copy(Environment *env, char **envp);
const char *env_find(Environment *env, const char *name);