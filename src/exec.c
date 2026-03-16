#include "42sh.h"
#include "ast.h"
#include "builtin.h"
#include "env.h"
#include "vec.h"
#include "utils.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static bool is_executable(const char *path) {
  struct stat st;
  if (stat(path, &st) == 0) {
    return (st.st_mode & S_IXUSR) != 0;
  }
  return false;
}

char *find_command_path(const char *cmd) {
  char *paths = env_find(environ, "PATH");
  if (!paths) {
    return NULL;
  }

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

void apply_assignments(Assignments *assignments) {
  if (!assignments || !vec_size(assignments)) {
    return;
  }

  vec_foreach(Assignment, assignment, assignments) {
    Variable *value = (Variable *)malloc(sizeof(Variable));
    value->content = strdup(assignment->value);
    value->exported = false;
    value->readonly = false;
    ht_insert(environ, assignment->name, value);
  }
}

void apply_redirs(Redirs *redirs) {
  if (!redirs || !vec_size(redirs)) {
    return;
  }

  vec_foreach(Redir, redir, redirs) {
    int fd = -1;

    switch (redir->type) {
    case REDIRECT_IN:
      fd = open(redir->path, O_RDONLY);
      if (fd == -1) {
        perror(redir->path);
        exit(EXIT_FAILURE);
      }
      dup2(fd, STDIN_FILENO);
      close(fd);
      break;
    case REDIRECT_HEREDOC:
      dup2(redir->fd, STDIN_FILENO);
      close(redir->fd);
      break;
    case REDIRECT_OUT:
      fd = open(redir->path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd == -1) {
        perror(redir->path);
        exit(EXIT_FAILURE);
      }
      dup2(fd, STDOUT_FILENO);
      close(fd);
      break;
    case REDIRECT_APPEND:
      fd = open(redir->path, O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (fd == -1) {
        perror(redir->path);
        exit(EXIT_FAILURE);
      }
      dup2(fd, STDOUT_FILENO);
      close(fd);
      break;
    case REDIRECT_OUT_FD:
      if (dup2(redir->fd, STDOUT_FILENO) == -1) {
        perror("dup2");
        exit(EXIT_FAILURE);
      }
      break;
    case REDIRECT_IN_FD:
      if (dup2(redir->fd, STDIN_FILENO) == -1) {
        perror("dup2");
        exit(EXIT_FAILURE);
      }
      break;
    }
  }
}

void execute_simple_command(AstNode *node) {
  if (!node || node->type != NODE_COMMAND) {
    return;
  }

  if (vec_size(&node->command.args) == 0 || is_builtin(node->command.args.data[0])) {
    exec_builtin(node);
    return;
  }

  char *path = NULL;
  
  CacheEntry *cached = hash_get(hash, node->command.args.data[0]);
  if (cached) {
    path = cached->path;
  } else {
    path = find_command_path(node->command.args.data[0]);
    if (!path) {
      fprintf(stderr, "42sh: %s: command not found\n", node->command.args.data[0]);
      exit_status = 127;
      return;
    }
    hash_insert(hash, node->command.args.data[0], path);
    free(path);

    path = hash_get(hash, node->command.args.data[0])->path;
  }

  pid_t pid = fork();
  if (pid < 0) {
    return;
  } else if (pid == 0) {
    char **args = node->command.args.data;

    apply_redirs(&node->command.redirs);
    apply_assignments(&node->command.assigns);

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = SIG_DFL;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);

    vec_push(&node->command.args, NULL);

    char **envp = env_to_cstr_array(environ);
    if (!envp) {
      exit(EXIT_FAILURE);
    }

    execve(path, args, envp);
    perror("execve");

    for (size_t i = 0; envp[i]; ++i) {
      free(envp[i]);
    }
    free(envp);

    cleanup();

    exit(EXIT_FAILURE);
  } else {
    int status = 0;
    waitpid(pid, &status, 0);

    CacheEntry *entry = hash_get(hash, node->command.args.data[0]);
    if (entry) {
      ++entry->hits;
    }

    if (WIFEXITED(status)) {
      exit_status = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
      exit_status = 128 + WTERMSIG(status);
    }

    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT) {
      write(STDOUT_FILENO, "\n", 1);
    }
  }
}

void execute_pipe(AstNode *node) {
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

    execute_command(node->operator.left);
    int status = exit_status;

    cleanup();

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

    execute_command(node->operator.right);
    int status = exit_status;

    cleanup();

    exit(status);
  }

  close(pipefd[0]);
  close(pipefd[1]);

  int status;
  waitpid(pid_left, NULL, 0);
  waitpid(pid_right, &status, 0);
  if (WIFEXITED(status)) {
    exit_status = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    exit_status = 128 + WTERMSIG(status);
  }
}

void execute_subshell(AstNode *node) {
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

    execute_command(node->group.inner);

    cleanup();

    exit(exit_status);
  } else {
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
      exit_status = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
      exit_status = 128 + WTERMSIG(status);
    }

    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT) {
      write(STDOUT_FILENO, "\n", 1);
    }
  }
}

void execute_command(AstNode *node) {
  if (!node) {
    return;
  }

  switch (node->type) {
  case NODE_AND:
    execute_command(node->operator.left);
    if (exit_status == 0) {
      execute_command(node->operator.right);
    }
    return;
  case NODE_OR:
    execute_command(node->operator.left);
    if (exit_status != 0) {
      execute_command(node->operator.right);
    }
    return;
  case NODE_PIPE: {
    execute_pipe(node);
    return;
  }
  case NODE_BACKGROUND:
  case NODE_SEMICOLON:
    execute_command(node->operator.left);
    execute_command(node->operator.right);
    return;
  case NODE_BRACE:
    execute_command(node->group.inner);
    return;
  case NODE_PAREN:
    execute_subshell(node);
    return;
  case NODE_COMMAND:
    execute_simple_command(node);
    return;
  }
}
