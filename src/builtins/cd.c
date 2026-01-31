#include "ast.h"
#include "env.h"
#include "vec.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_PATH 4096

void builtin_cd(AstNode *node, Environment *env) {
  char oldpwd[MAX_PATH];
  char pwd[MAX_PATH];

  if (vec_size(&node->command.args) > 2) {
    return;
  }

  char *path = NULL;
  if (vec_size(&node->command.args) == 1) {
    path = env_find(env, "HOME");
    if (!path) {
      return;
    }
  } else {
    path = node->command.args.data[1];
    if (!strcmp("-", path)) {
      path = env_find(env, "OLDPWD");
      if (!path) {
        return;
      }
    }
  }

  if (!path) {
    return;
  }

  if (!getcwd(oldpwd, MAX_PATH)) {
    return;
  }
  if (chdir(path) == -1) {
    printf("Error\n");
    return;
  }
  if (!getcwd(pwd, MAX_PATH)) {
    return;
  }

  env_set(env, "OLDPWD", oldpwd);
  env_set(env, "PWD", pwd);
}
