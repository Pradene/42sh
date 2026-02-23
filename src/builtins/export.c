#include "env.h"
#include "ast.h"
#include "vec.h"

#include <stdio.h>
#include <ctype.h>

void builtin_export(AstNode *node, Variables *env) {
  char **args = node->command.args.data;
  size_t argc = vec_size(&node->command.args);

  if (argc == 1) {
    env_print(env);
    return;
  }

  for (size_t i = 1; i < argc; ++i) {
    char *arg = args[i];

    if (!(isalpha(arg[0]) || arg[0] == '_')) {
      fprintf(stderr, "export: %s: not a valid identifier\n", arg);
      continue;
    }

    char *equal_sign = strchr(arg, '=');
    if (!equal_sign) {
      continue;
    } else {
      size_t key_length = equal_sign - arg;
      char *key = strndup(arg, key_length);
      char *value = strdup(equal_sign + 1);
      
      if (strcmp(value, "") == 0) {
        free(key);
        free(value);
        continue;
      }

      env_set(env, key, value);

      free(key);
      free(value);
    } 
  }
}
