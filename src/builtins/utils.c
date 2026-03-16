#include "builtin.h"
#include "env.h"
#include "ast.h"
#include "vec.h"

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
  { "hash", builtin_hash },
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

void exec_builtin(AstNode *node) {
  if (!node || node->type != NODE_COMMAND)
    return;

  int saved_stdin  = dup(STDIN_FILENO);
  int saved_stdout = dup(STDOUT_FILENO);
  int saved_stderr = dup(STDERR_FILENO);

  bool has_command = vec_size(&node->command.args) != 0;
  Assignments *assigns = &node->command.assigns;
  Redirs *redirs = &node->command.redirs;

  Variable **old_values = NULL;
  if (has_command && vec_size(assigns)) {
    old_values = calloc(vec_size(assigns), sizeof(Variable *));
    for (size_t i = 0; i < vec_size(assigns); ++i) {
      const char *name  = assigns->data[i].name;
      HtEntry  *entry = ht_get(environ, name);
      if (entry) {
        Variable *v    = entry->value;
        Variable *copy = malloc(sizeof(Variable));
        copy->content  = strdup(v->content);
        copy->exported = v->exported;
        copy->readonly = v->readonly;
        old_values[i]  = copy;
      }
    }
  }

  apply_redirs(redirs);
  apply_assignments(assigns);

  if (has_command) {
    char *command = node->command.args.data[0];
    for (int i = 0; builtins[i].name; ++i) {
      if (!strcmp(command, builtins[i].name)) {
        builtins[i].fn(node);
        break;
      }
    }

    for (size_t i = 0; i < vec_size(assigns); ++i) {
      const char *name = assigns->data[i].name;
      if (old_values[i]) {
        ht_insert(environ, name, old_values[i]);
      } else {
        ht_remove(environ, name);
      }
    }

    free(old_values);
  }

  dup2(saved_stdin,  STDIN_FILENO);
  dup2(saved_stdout, STDOUT_FILENO);
  dup2(saved_stderr, STDERR_FILENO);

  close(saved_stdin);
  close(saved_stdout);
  close(saved_stderr);
}