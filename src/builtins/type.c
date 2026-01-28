#include "builtin.h"
#include "env.h"
#include "ast.h"
#include "42sh.h"
#include "vec.h"

#include <unistd.h>
#include <stdio.h>

void builtin_type(AstNode *node, Environment *env) {
  char **args = node->command.args.data;
  size_t argc = vec_size(&node->command.args);

  if (argc < 2) {
    write(2, "type: too few arguments\n", 24);
    return;
  }

  for (size_t i = 1; i < argc; ++i) {
    char *cmd = args[i];
    if (is_builtin(cmd)) {
      printf("%s is a shell builtin\n", cmd);
    } else {
      char *path = find_command_path(cmd, env);
      if (path) {
        printf("%s is %s\n", cmd, path);
        free(path);
      } else {
        printf("type: %s: not found\n", cmd);
      }
    }
  }
}