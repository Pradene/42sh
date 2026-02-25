#include "builtin.h"
#include "env.h"
#include "ast.h"
#include "vec.h"
#include <stdio.h>

void builtin_unalias(AstNode *node, Shell *shell) {
  size_t argc = vec_size(&node->command.args);
  if (argc == 1) {
    fprintf(stderr, "unalias: usage: unalias name [name ...]\n");
    return;
  }

  char **args = node->command.args.data;
  for (size_t i = 1; i < argc; ++i) {
    ht_remove(&shell->aliases, args[i]);
  }
}