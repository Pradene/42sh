#include "ast.h"
#include "env.h"
#include "vec.h"
#include "builtin.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_PATH 4096

void builtin_cd(AstNode *node) {
  size_t argc = vec_size(&node->command.args);
  if (argc > 2) {
    exit_status = 1;
    return;
  }

  char *old_path = malloc(sizeof(char) * MAX_PATH);
  char *new_path = malloc(sizeof(char) * MAX_PATH);

  char *path = NULL;
  if (argc == 1) {
    char *variable = env_find(environ, "HOME");
    path = variable;
  } else {
    path = node->command.args.data[1];
    if (!strcmp("-", path)) {
      char *variable = env_find(environ, "OLDPWD");
      path = variable;
    }
  }

  if (!path) {
    exit_status = 1;
    return;
  }

  if (!getcwd(old_path, MAX_PATH)) {
    exit_status = 2;
    return;
  }
  if (chdir(path) == -1) {
    exit_status = 2;
    return;
  }
  if (!getcwd(new_path, MAX_PATH)) {
    exit_status = 2;
    return;
  }

  Variable old = (Variable){
    .content = old_path,
    .exported = true,
    .readonly = false,
  };
  ht_insert(environ, "OLDPWD", &old, sizeof(Variable));
  
  Variable new = (Variable){
    .content = new_path,
    .exported = true,
    .readonly = false,
  };
  ht_insert(environ, "PWD", &new, sizeof(Variable));
}
