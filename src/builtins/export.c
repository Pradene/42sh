#include "env.h"
#include "ast.h"
#include "vec.h"
#include "builtin.h"

#include <stdio.h>
#include <ctype.h>

void builtin_export(AstNode *node, Shell *shell) {
  char **args = node->command.args.data;
  size_t argc = vec_size(&node->command.args);

  if (argc == 1) {
    env_print(&shell->environment);
    return;
  }

  for (size_t i = 1; i < argc; ++i) {
    char *arg = args[i];

    if (!(isalpha(arg[0]) || arg[0] == '_')) {
      fprintf(stderr, "export: %s: not a valid identifier\n", arg);
      continue;
    }

    char *equal = strchr(arg, '=');
    if (!equal) {
      continue;
    } else {
      size_t key_length = equal - arg;
      char *key = strndup(arg, key_length);
      char *content = strdup(equal + 1);
      if (strcmp(content, "") == 0) {
        free(key);
        free(content);
        continue;
      }

      Variable *value = (Variable *)malloc(sizeof(Variable));
      value->content = content;
      value->exported = true;
      value->readonly = false;
      env_set(&shell->environment, key, value);

      free(key);
    }
  }
}
