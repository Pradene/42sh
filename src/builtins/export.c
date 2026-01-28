#include "env.h"
#include "ast.h"
#include "vec.h"

void builtin_export(AstNode *node, Environment *env) {
  char **args = node->command.args.data;
  size_t argc = vec_size(&node->command.args);

  if (argc == 1) {
    env_print(env);
    return;
  }

  for (size_t i = 1; i < argc; ++i) {
    char *arg = args[i];
    char *equal_sign = strchr(arg, '=');

    if (equal_sign) {
      size_t key_length = equal_sign - arg;
      char *key = strndup(arg, key_length);
      char *value = strdup(equal_sign + 1);

      env_set(env, key, value);

      free(key);
      free(value);
    } else {
      env_set(env, arg, "");
    }
  }
}
