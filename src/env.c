#include "42sh.h"
#include "vec.h"

StatusCode env_copy(Environment *env, char **envp) {
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
  for (size_t i = 0; i < vec_size(env); ++i) {
    free(vec_at(env, i));
  }
  vec_free(env);
}
