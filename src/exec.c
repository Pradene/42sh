#include "42sh.h"

#include "ast.h"
#include "env.h"
#include "vec.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

static bool is_executable(const char *path) {
  struct stat st;
  if (stat(path, &st) == 0) {
    return (st.st_mode & S_IXUSR) != 0;
  }
  return false;
}

static char *find_command_path(const char *cmd, Environment *env) {
  if (strchr(cmd, '/')) {
    if (is_executable(cmd)) {
      return strdup(cmd);
    }
    return NULL;
  }

  const char *env_path = env_find(env, "PATH");
  if (!env_path) {
    return NULL;
  }

  char *copy = strdup(env_path);
  if (!copy) {
    return NULL;
  }

  char *result = NULL;
  char *save = NULL;
  char *dir = strtok_r(copy, ":", &save);
  
  while (dir) {
    size_t length = strlen(dir) + strlen(cmd) + 2;
    char *path = malloc(length);
    if (!path) {
      break;
    }

    snprintf(path, length, "%s/%s", dir, cmd);

    if (is_executable(path)) {
      result = path;
      break;
    }
    
    free(path);
    dir = strtok_r(NULL, ":", &save);
  }

  free(copy);
  return result;
}

void execute_simple_command(AstNode *node, Environment *env) {
  if (!node || node->type != NODE_COMMAND) {
    return;
  }

  pid_t pid = fork();
  if (pid < 0) {
    return;
  } else if (pid == 0) {
    char **args = node->command.args.data;
    char *path = find_command_path(args[0], env);
    if (!path) {
      fprintf(stderr, "Command not found: %s\n", args[0]);
      exit(EXIT_FAILURE);
    } else {
      env_set(env, "_", path);
    }

    vec_push(env, NULL);
    vec_push(&node->command.args, NULL);

    execve(path, args, env->data);

    free(path);
    exit(EXIT_FAILURE);
  } else {
    int status;
    waitpid(pid, &status, 0);
  }
}

void execute_command(AstNode *root, Environment *env) {
  if (!root) {
    return;
  }

  switch (root->type) {
  case NODE_AND:
    execute_command(root->operator.left, env);
    // Change to exit code of left command
    if (true == true) {
      execute_command(root->operator.right, env);
    }
    return;
  case NODE_OR:
    execute_command(root->operator.left, env);
    // Change to exit code of left command
    if (false == false) {
      execute_command(root->operator.right, env);
    }
    return;
  case NODE_PIPE:
  case NODE_BACKGROUND:
  case NODE_SEMICOLON:
    execute_command(root->operator.left, env);
    execute_command(root->operator.right, env);
    return;
  case NODE_BRACE:
  case NODE_PAREN:
    expansion(root, env);
    stripping(root);
    execute_command(root->group.inner, env);
    return;
  case NODE_COMMAND:
    expansion(root, env);
    stripping(root);
    execute_simple_command(root, env);
    return;
  }
}