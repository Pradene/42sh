#pragma once

#include <stdlib.h>

#include "status.h"

typedef struct {
  char *name;
  char *value;

} Variable;

typedef struct {
  Variable *data;
  size_t size;
  size_t capacity;
} Variables;

void env_free(Variables *env);
StatusCode env_from_cstr_array(Variables *env, const char **envp);
char **env_to_cstr_array(const Variables *env);
Variable *env_find(const Variables *env, const char *name);
StatusCode env_set(Variables *env, const char *name, const char *value);
StatusCode env_unset(Variables *env, const char *name);
void env_print(const Variables *env);
