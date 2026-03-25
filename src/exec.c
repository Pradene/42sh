#include "42sh.h"
#include "ast.h"
#include "builtin.h"
#include "env.h"
#include "vec.h"
#include "utils.h"
#include "jobs.h"

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
    Variable value = (Variable){
      .content = strdup(assignment->value),
      .exported = false,
      .readonly = false,
    };
    ht_insert(environ, assignment->name, &value);
  }
}

bool apply_redirs(Redirs *redirs) {
  if (!redirs || !vec_size(redirs)) {
    return true;
  }

  vec_foreach(Redir, redir, redirs) {
    int fd = -1;

    switch (redir->type) {
    case REDIRECT_IN:
      fd = open(redir->path, O_RDONLY);
      if (fd == -1) {
        perror(redir->path);
        return false;
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
        return false;
      }
      dup2(fd, STDOUT_FILENO);
      close(fd);
      break;
    case REDIRECT_APPEND:
      fd = open(redir->path, O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (fd == -1) {
        perror(redir->path);
        return false;
      }
      dup2(fd, STDOUT_FILENO);
      close(fd);
      break;
    case REDIRECT_OUT_FD:
      if (dup2(redir->fd, STDOUT_FILENO) == -1) {
        perror("dup2");
        return false;
      }
      break;
    case REDIRECT_IN_FD:
      if (dup2(redir->fd, STDIN_FILENO) == -1) {
        perror("dup2");
        return false;
      }
      break;
    default:
      return false;
    }
  }

  return true;
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

    setpgid(0, 0);
    signals_reset();

    apply_assignments(&node->command.assigns);
    if (!apply_redirs(&node->command.redirs)) {
      exit(EXIT_FAILURE);
    }

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
    setpgid(pid, pid);

    Job *job = job_add(&jobs, pid, pid, node->command.args.data[0]);
    
    if (is_interactive) {
      signal(SIGTTOU, SIG_IGN);
      tcsetpgrp(STDIN_FILENO, pid);
    }

    int status = 0;
    waitpid(pid, &status, WUNTRACED);
    
    if (is_interactive) {
      tcsetpgrp(STDIN_FILENO, getpgrp());
      signal(SIGTTOU, SIG_DFL);
    }

    CacheEntry *entry = hash_get(hash, node->command.args.data[0]);
    if (entry) {
      ++entry->hits;
    }

    if (WIFSTOPPED(status)) {
      tcgetattr(STDIN_FILENO, &job->tmodes);
      job->status   = JOB_STOPPED;
      job->notified = true;
      fprintf(stderr, "\n[%d]+  Stopped\t\t%s\n", job->id, job->command);
    } else {
      if (WIFEXITED(status)) {
        exit_status = WEXITSTATUS(status);
      } else if (WIFSIGNALED(status)) {
        exit_status = 128 + WTERMSIG(status);
      }

      if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT) {
        write(STDOUT_FILENO, "\n", 1);
      }

      job_remove(&jobs, job);
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
    setpgid(0, 0);

    signals_reset();

    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);

    execute_command(node->operator.left);

    cleanup();
    exit(exit_status);
  }

  pid_t pid_right = fork();
  if (pid_right < 0) {
    perror("fork");
    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid_left, NULL, 0);
    return;
  } else if (pid_right == 0) {
    setpgid(0, pid_left);

    signals_reset();

    close(pipefd[1]);
    dup2(pipefd[0], STDIN_FILENO);
    close(pipefd[0]);

    execute_command(node->operator.right);

    cleanup();
    exit(exit_status);
  }

  close(pipefd[0]);
  close(pipefd[1]);

  setpgid(pid_left, pid_left);
  setpgid(pid_right, pid_left);

  Job *job = job_add(&jobs, pid_left, pid_left, "pipeline");
  job_add_pid(job, pid_right);

  if (is_interactive) {
    signal(SIGTTOU, SIG_IGN);
    tcsetpgrp(STDIN_FILENO, pid_left);
  }

  int status_left;
  int status_right;

  waitpid(pid_left, &status_left, WUNTRACED);
  waitpid(pid_right, &status_right, WUNTRACED);

  if (is_interactive) {
    tcsetpgrp(STDIN_FILENO, getpgrp());
    signal(SIGTTOU, SIG_DFL);
  }

  if (WIFSTOPPED(status_left) || WIFSTOPPED(status_right)) {
    tcgetattr(STDIN_FILENO, &job->tmodes);
    job->status   = JOB_STOPPED;
    job->notified = true;
    fprintf(stderr, "\n[%d]+  Stopped\t\t%s\n", job->id, job->command);
  } else {
    if (WIFEXITED(status_right)) {
      exit_status = WEXITSTATUS(status_right);
    } else if (WIFSIGNALED(status_right)) {
      exit_status = 128 + WTERMSIG(status_right);
    }

    job_remove(&jobs, job);
  }
}

void execute_subshell(AstNode *node) {
  pid_t pid = fork();

  if (pid < 0) {
    return;
  } else if (pid == 0) {
    setpgid(0, 0);
    signals_reset();

    if (!apply_redirs(&node->group.redirs)) {
      exit(EXIT_FAILURE);
    }

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

void execute_background(AstNode *node) {
  pid_t pgid = 0;
  const char *cmd = "job";

  if (node->type == NODE_COMMAND) {
    char *path = NULL;
    if (node->command.args.size == 0) {
      return;
    }

    cmd = node->command.args.data[0];

    CacheEntry *cached = hash_get(hash, cmd);
    if (cached) {
      path = cached->path;
    } else {
      path = find_command_path(cmd);
      if (!path) {
        fprintf(stderr, "42sh: %s: command not found\n", cmd);
        exit_status = 127;
        return;
      }
      hash_insert(hash, cmd, path);
      free(path);
      path = hash_get(hash, cmd)->path;
    }

    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      return;
    } else if (pid == 0) {
      setpgid(0, 0);
      signals_reset();

      apply_assignments(&node->command.assigns);
      if (!apply_redirs(&node->command.redirs)) {
        exit(EXIT_FAILURE);
      }
      
      vec_push(&node->command.args, NULL);
      char **envp = env_to_cstr_array(environ);
      if (!envp) {
        exit(EXIT_FAILURE);
      }
      
      execve(path, node->command.args.data, envp);
      perror("execve");
      exit(EXIT_FAILURE);
    }

    setpgid(pid, pid);
    pgid = pid;
    Job *job = job_add(&jobs, pgid, pid, cmd);
    fprintf(stderr, "[%d] %d\n", job->id, pid);

  } else if (node->type == NODE_PIPE) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
      perror("pipe");
      return;
    }

    pid_t pid_left = fork();
    if (pid_left < 0) {
      close(pipefd[0]); close(pipefd[1]);
      return;
    } else if (pid_left == 0) {
      setpgid(0, 0);
      signals_reset();
      close(pipefd[0]);
      dup2(pipefd[1], STDOUT_FILENO);
      close(pipefd[1]);
      execute_command(node->operator.left);
      cleanup();
      exit(exit_status);
    }

    pid_t pid_right = fork();
    if (pid_right < 0) {
      close(pipefd[0]); close(pipefd[1]);
      waitpid(pid_left, NULL, 0);
      return;
    } else if (pid_right == 0) {
      setpgid(0, pid_left);
      signals_reset();
      close(pipefd[1]);
      dup2(pipefd[0], STDIN_FILENO);
      close(pipefd[0]);
      execute_command(node->operator.right);
      cleanup();
      exit(exit_status);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    setpgid(pid_left, pid_left);
    setpgid(pid_right, pid_left);
    pgid = pid_left;

    Job *job = job_add(&jobs, pgid, pid_left, "pipeline");
    job_add_pid(job, pid_right);
    fprintf(stderr, "[%d] %d\n", job->id, pid_left);
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
  case NODE_PIPE:
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
    command_substitution(node);
    if (!variable_expansion(node)) {
      return;
    }
    word_splitting(node);
    quotes_removal(node);
    execute_simple_command(node);
    return;
  }
}
