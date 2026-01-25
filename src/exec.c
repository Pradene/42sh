#include "42sh.h"

#include "ast.h"
#include "env.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void execute_simple_command(AstNode *node, Environment *env) {
  if (!node || node->type != NODE_COMMAND) {
    return;
  }

  pid_t pid = fork();
  if (pid < 0) {
    return; 
  } else if (pid == 0) {
    char **args = node->command.args.data;
    execve(args[0], args, env->data);
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
  case NODE_PIPE:
  case NODE_AND:
  case NODE_OR:
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