#include "42sh.h"

#include "ast.h"
#include "builtin.h"
#include "env.h"
#include "vec.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int g_status = 0;

static bool is_executable(const char *path) {
  struct stat st;
  if (stat(path, &st) == 0) {
    return (st.st_mode & S_IXUSR) != 0;
  }
  return false;
}

char *find_command_path(const char *cmd, const char *paths) {
  if (strchr(cmd, '/')) {
    if (is_executable(cmd)) {
      return strdup(cmd);
    }
    return NULL;
  }

  char *copy = strdup(paths);
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
    case REDIRECT_OUT_FD:
      if (dup2(redir->target_fd, STDOUT_FILENO) == -1) {
        perror("dup2");
        exit(EXIT_FAILURE);
      }
      break;
    case REDIRECT_IN_FD:
      if (dup2(redir->target_fd, STDIN_FILENO) == -1) {
        perror("dup2");
        exit(EXIT_FAILURE);
      }
      break;
    }
  }
}

void execute_simple_command(AstNode *node, Shell *shell) {
  if (!node || node->type != NODE_COMMAND) {
    return;
  }

  if (is_builtin(node->command.args.data[0])) {
    exec_builtin(node, shell);
    return;
  }

  pid_t pid = fork();
  if (pid < 0) {
    return;
  } else if (pid == 0) {
    char **args = node->command.args.data;
    char *paths = env_find(&shell->environment, "PATH");
    if (!paths) {
      fprintf(stderr, "PATH is not set\n");
      exit(EXIT_FAILURE);
    }
    
    char *path = find_command_path(args[0], paths);
    if (!path) {
      fprintf(stderr, "Command not found: %s\n", args[0]);
      ast_free(shell->command);
      ht_clear(&shell->environment);
      ht_clear(&shell->aliases);
      exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = SIG_DFL;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);

    apply_redirs(&node->command.redirs);

    vec_push(&node->command.args, NULL);

    char **envp = env_to_cstr_array(&shell->environment);
    if (!envp) {
      exit(EXIT_FAILURE);
    }

    execve(path, args, envp);
    perror("execve");

    for (size_t i = 0; envp[i]; ++i) {
      free(envp[i]);
    }
    free(envp);
    free(path);
    ast_free(shell->command);
    ht_clear(&shell->environment);
    ht_clear(&shell->aliases);

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

void execute_pipe(AstNode *root, Shell *shell) {
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

    execute_command(root->operator.left, shell);
    int status = g_status;

    ast_free(shell->command);
    ht_clear(&shell->environment);
    ht_clear(&shell->aliases);

    exit(status);
  }

  pid_t pid_right = fork();
  if (pid_right < 0) {
    perror("fork");
    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid_left, NULL, 0);
    return;
  } else if (pid_right == 0) {
    close(pipefd[1]);
    dup2(pipefd[0], STDIN_FILENO);
    close(pipefd[0]);

    execute_command(root->operator.right, shell);
    int status = g_status;

    ast_free(shell->command);
    ht_clear(&shell->environment);
    ht_clear(&shell->aliases);

    exit(status);
  }

  close(pipefd[0]);
  close(pipefd[1]);

  int status;
  waitpid(pid_left, NULL, 0);
  waitpid(pid_right, &status, 0);
  if (WIFEXITED(status)) {
    g_status = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    g_status = 128 + WTERMSIG(status);
  }
}

void execute_subshell(AstNode *node, Shell *shell) {
  pid_t pid = fork();

  if (pid < 0) {
    return;
  } else if (pid == 0) {
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = SIG_DFL;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);

    apply_redirs(&node->group.redirs);

    execute_command(node->group.inner, shell);

    ast_free(node);
    ht_clear(&shell->environment);
    ht_clear(&shell->aliases);

    exit(g_status);
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

void execute_command(AstNode *root, Shell *shell) {
  if (!root) {
    return;
  }

  switch (root->type) {
  case NODE_AND:
    execute_command(root->operator.left, shell);
    if (g_status == 0) {
      execute_command(root->operator.right, shell);
    }
    return;
  case NODE_OR:
    execute_command(root->operator.left, shell);
    if (g_status != 0) {
      execute_command(root->operator.right, shell);
    }
    return;
  case NODE_PIPE: {
    execute_pipe(root, shell);
    return;
  }
  case NODE_BACKGROUND:
  case NODE_SEMICOLON:
    execute_command(root->operator.left, shell);
    execute_command(root->operator.right, shell);
    return;
  case NODE_BRACE:
    expansion(root, shell);
    stripping(root);
    execute_command(root->group.inner, shell);
    return;
  case NODE_PAREN:
    expansion(root, shell);
    stripping(root);
    execute_subshell(root, shell);
    return;
  case NODE_COMMAND:
    expansion(root, shell);
    word_splitting(root);
    stripping(root);
    execute_simple_command(root, shell);
    return;
  }
}
