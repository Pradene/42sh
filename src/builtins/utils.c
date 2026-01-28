#include "builtin.h"
#include "env.h"
#include "ast.h"

#include <string.h>
#include <stdbool.h>

typedef void (*builtin)(AstNode *, Environment *);
typedef struct {
  const char *name;
  builtin fn;
} Builtin;

static Builtin builtins[] = {
  { "echo", builtin_echo },
  { "exit", builtin_exit },
  { "export", builtin_export },
  { "type", builtin_type },
  { NULL, NULL }
};

bool is_builtin(const char *command) {
  if (!command) {
    return false;
  }

  for (size_t i = 0; builtins[i].name; ++i) {
    if (!strcmp(command, builtins[i].name)) {
      return true;
    }
  }

  return false;
}

void exec_builtin(AstNode *node, Environment *env) {
  if (!node || node->type != NODE_COMMAND) {
    return;
  }

  char *command = node->command.args.data[0];
  if (!command) {
    return;
  }

  for (int i = 0; builtins[i].name; ++i) {
    if (!strcmp(command, builtins[i].name)) {
      builtins[i].fn(node, env);
      return;
    }
  }
}
