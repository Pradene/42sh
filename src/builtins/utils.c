#include "builtin.h"
#include "env.h"
#include "ast.h"

#include <string.h>
#include <stdbool.h>
#include <unistd.h>

static Builtin builtins[] = {
  { "alias", builtin_alias },
  { "unalias", builtin_unalias },
  { "echo", builtin_echo },
  { "exit", builtin_exit },
  { "export", builtin_export },
  { "type", builtin_type },
  { "cd", builtin_cd },
  { "set", builtin_set },
  { "unset", builtin_unset },
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

void exec_builtin(AstNode *node, Shell *shell) {
  if (!node || node->type != NODE_COMMAND)
    return;

  char *command = node->command.args.data[0];
  if (!command)
    return;

  int saved_stdin  = dup(STDIN_FILENO);
  int saved_stdout = dup(STDOUT_FILENO);
  int saved_stderr = dup(STDERR_FILENO);

  apply_redirs(&node->command.redirs);

  for (int i = 0; builtins[i].name; ++i) {
    if (!strcmp(command, builtins[i].name)) {
      builtins[i].fn(node, shell);
      break;
    }
  }

  dup2(saved_stdin,  STDIN_FILENO);
  dup2(saved_stdout, STDOUT_FILENO);
  dup2(saved_stderr, STDERR_FILENO);

  close(saved_stdin);
  close(saved_stdout);
  close(saved_stderr);
}
