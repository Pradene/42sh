#include "42sh.h"

#include "ast.h"
#include "env.h"
#include "vec.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

static int g_status = 0;

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

static void apply_redirs(Redirs *redirs) {
  if (!redirs || !vec_size(redirs)) {
    return;
  }

  vec_foreach(Redir, redir, redirs) {
    int fd = -1;

    switch (redir->type) {
    case REDIRECT_IN:
      fd = open(redir->target_path, O_RDONLY);
      if (fd == -1) {
        perror(redir->target_path);
        exit(EXIT_FAILURE);
      }
      dup2(fd, STDIN_FILENO);
      close(fd);
      break;
    case REDIRECT_HEREDOC:
      dup2(redir->target_fd, STDIN_FILENO);
      close(redir->target_fd);
      break;
    case REDIRECT_OUT:
      fd = open(redir->target_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd == -1) {
        perror(redir->target_path);
        exit(EXIT_FAILURE);
      }
      dup2(fd, STDOUT_FILENO);
      close(fd);
      break;
    case REDIRECT_APPEND:
      fd = open(redir->target_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (fd == -1) {
        perror(redir->target_path);
        exit(EXIT_FAILURE);
      }
      dup2(fd, STDOUT_FILENO);
      close(fd);
      break;
    }
  }
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

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = SIG_DFL;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);

    apply_redirs(&node->command.redirs);

    vec_push(env, NULL);
    vec_push(&node->command.args, NULL);

    execve(path, args, env->data);

    free(path);
    exit(EXIT_FAILURE);
  } else {
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
      g_status = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
      g_status = 128 + WTERMSIG(status);
    }

    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT) {
      write(STDOUT_FILENO, "\n", 1);
    }
  }
}

void execute_pipe(AstNode *root, Environment *env) {
  int pipefd[2];
  if (pipe(pipefd) == -1) {
    perror("pipe");
    return;
  }

  pid_t pid_left = fork();
  if (pid_left < 0) {
    perror("fork");
    close(pipefd[0]);
    close(pipefd[1]);
    return;
  } else if (pid_left == 0) {
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);

    execute_command(root->operator.left, env);
    exit(g_status);
  }

  pid_t pid_right = fork();
  if (pid_right < 0) {
    perror("fork");
    close(pipefd[0]);
    close(pipefd[1]);
    return;
  } else if (pid_right == 0) {
    close(pipefd[1]);
    dup2(pipefd[0], STDIN_FILENO);
    close(pipefd[0]);

    execute_command(root->operator.right, env);
    exit(g_status);
  }

  close(pipefd[0]);
  close(pipefd[1]);

  int status;
  waitpid(pid_left, &status, 0);
  waitpid(pid_right, &status, 0);
}

void execute_command(AstNode *root, Environment *env) {
  if (!root) {
    return;
  }

  switch (root->type) {
  case NODE_AND:
    execute_command(root->operator.left, env);
    if (g_status == 0) {
      execute_command(root->operator.right, env);
    }
    return;
  case NODE_OR:
    execute_command(root->operator.left, env);
    if (g_status != 0) {
      execute_command(root->operator.right, env);
    }
    return;
  case NODE_PIPE: {
    execute_pipe(root, env);
    return;
  }
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