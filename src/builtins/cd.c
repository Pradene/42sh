#include "ast.h"
#include "env.h"
#include "vec.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_PATH 4096

void builtin_cd(AstNode *node, HashTable *env) {
  char oldpwd[MAX_PATH];
  char pwd[MAX_PATH];

  if (vec_size(&node->command.args) > 2) {
    return;
  }

  char *path = NULL;
  if (vec_size(&node->command.args) == 1) {
    char *variable = env_find(env, "HOME");
    path = variable;
  } else {
    path = node->command.args.data[1];
    if (!strcmp("-", path)) {
      char *variable = env_find(env, "OLDPWD");
      path = variable;
    }
  }

  if (!path) {
    return;
  }

  if (!getcwd(oldpwd, MAX_PATH)) {
    return;
  }
  if (chdir(path) == -1) {
    return;
  }
  if (!getcwd(pwd, MAX_PATH)) {
    return;
  }

  env_set(env, "OLDPWD", oldpwd);
  env_set(env, "PWD", pwd);
}
