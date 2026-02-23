#include "env.h"
#include "vec.h"
#include "utils.h"

#include <stdio.h>

StatusCode env_from_cstr_array(Variables *env, const char **envp) {
  for (size_t i = 0; envp[i]; ++i) {
    char **parts = split_at(envp[i], '=');
    if (!parts || !parts[0] || !parts[1]) {
      return MEM_ALLOCATION_FAILED;
    }
    Variable variable = { .name = parts[0], .value = parts[1] };
    vec_push(env, variable);
  }

  return OK;
}

char **env_to_cstr_array(const Variables *env) {
  char **array = malloc((env->size + 1) * sizeof(char *));
  if (!array) {
    return NULL;
  }

  for (size_t i = 0; i < env->size; ++i) {
    size_t length = strlen(env->data[i].name) + strlen(env->data[i].value) + 2;
    array[i] = malloc(length);
    if (!array[i]) {
      for (size_t j = 0; j < i; ++j) {
        free(array[j]);
      }
      free(array);
      return NULL;
    }
    snprintf(array[i], length, "%s=%s", env->data[i].name, env->data[i].value);
  }
  array[env->size] = NULL;

  return array;
}

void env_free(Variables *env) {
  vec_foreach(Variable, v, env) {
    free(v->name);
    free(v->value);
  }
  vec_free(env);
}

Variable *env_find(const Variables *env, const char *name) {
  vec_foreach(Variable, v, env) {
    if (strcmp(v->name, name) == 0) {
      return v;
    }
  }
  return NULL;
}

StatusCode env_set(Variables *env, const char *name, const char *value) {
  Variable *variable = env_find(env, name);
  if (variable) {
    free(variable->value);
    variable->value = strdup(value);
  } else {
    Variable new = { .name = strdup(name), .value = strdup(value) };
    if (!new.name || !new.value) {
      if (new.name) {
        free(new.name);
      }
      if (new.value) {
        free(new.value);
      }
      return MEM_ALLOCATION_FAILED;
    }
    vec_push(env, new);
  }
  return OK;
}

StatusCode env_unset(Variables *env, const char *name) {
  vec_foreach(Variable, v, env) {
    if (strcmp(v->name, name) == 0) {
      free(v->name);
      free(v->value);
      *v = env->data[env->size - 1];
      env->size--;
      return OK;
    }
  }
  return OK;
}

void env_print(const Variables *env) {
  vec_foreach(Variable, v, env) { printf("%s=%s\n", v->name, v->value); }
}
