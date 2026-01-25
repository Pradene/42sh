#include "env.h"
#include "vec.h"

StatusCode env_copy(Environment *env, const char **envp) {
  for (size_t i = 0; envp[i]; ++i) {
    char *variable = strdup(envp[i]);
    if (!variable) {
      return MEM_ALLOCATION_FAILED;
    }
    vec_push(env, variable);
  }

  return OK;
}

void env_free(Environment *env) {
  vec_foreach(char *,  var, env) {
    free(*var);
  }
  vec_free(env);
}

const char *env_find(const Environment *env, const char *name) {
  size_t name_length = strlen(name);
  vec_foreach(char *, var, env) {
    if (strncmp(*var, name, name_length) == 0 && (*var)[name_length] == '=') {
      return *var + name_length + 1;
    }
  }
  return NULL;
}