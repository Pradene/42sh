#include "builtin.h"
#include "env.h"
#include "vec.h"

#include <stdio.h>

void builtin_unset(AstNode *node, Shell *shell) {
  size_t argc = vec_size(&node->command.args);
  if (argc == 1) {
    fprintf(stderr, "unset: usage: unset name [name ...]\n");
    return;
  }

  char **args = node->command.args.data;
  for (size_t i = 1; i < argc; ++i) {
    char *name = args[i];
    env_unset(&shell->environment, name);
  }
}