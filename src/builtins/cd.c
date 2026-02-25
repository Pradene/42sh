#include "ast.h"
#include "env.h"
#include "vec.h"
#include "builtin.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_PATH 4096

void builtin_cd(AstNode *node, Shell *shell) {
  size_t argc = vec_size(&node->command.args);
  if (argc > 2) {
    shell->status = 1;
    return;
  }

  char *old_path = malloc(sizeof(char) * MAX_PATH);
  char *new_path = malloc(sizeof(char) * MAX_PATH);

  char *path = NULL;
  if (argc == 1) {
    char *variable = env_find(&shell->environment, "HOME");
    path = variable;
  } else {
    path = node->command.args.data[1];
    if (!strcmp("-", path)) {
      char *variable = env_find(&shell->environment, "OLDPWD");
      path = variable;
    }
  }

  if (!path) {
    shell->status = 1;
    return;
  }

  if (!getcwd(old_path, MAX_PATH)) {
    shell->status = 2;
    return;
  }
  if (chdir(path) == -1) {
    shell->status = 2;
    return;
  }
  if (!getcwd(new_path, MAX_PATH)) {
    shell->status = 2;
    return;
  }

  Variable *old = (Variable *)malloc(sizeof(Variable));
  old->content = old_path;
  old->exported = true;
  old->readonly = false;
  ht_insert(&shell->environment, strdup("OLDPWD"), old);
  
  Variable *new = (Variable *)malloc(sizeof(Variable));
  new->content = new_path;
  new->exported = true;
  new->readonly = false;
  ht_insert(&shell->environment, strdup("PWD"), new);
}
