#include "42sh.h"
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "sb.h"
#include "status.h"
#include "utils.h"

#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>

static int run_string(const char *input, Shell *shell) {
  AstNode *root = NULL;
  StatusCode status = parse(input, &root);

  if (status == INCOMPLETE_INPUT) {
    fprintf(stderr, "42sh: unexpected end of input\n");
    return 2;
  }
  if (status != OK || !root) {
    fprintf(stderr, "42sh: syntax error\n");
    return 2;
  }

  shell->command = root;
  execute_command(root, shell);
  ast_free(root);
  shell->command = NULL;

  return shell->status;
}

static int run_fd(int fd, const char *name, Shell *shell) {
  StringBuffer sb = {0};
  char buf[4096];
  ssize_t n;

  while ((n = read(fd, buf, sizeof(buf) - 1)) > 0) {
    buf[n] = '\0';
    sb_append(&sb, buf);
  }

  char *content = sb_as_cstr(&sb);
  sb_free(&sb);

  if (!content) {
    return 1;
  }

  StringBuffer acc = {0};
  size_t i = 0;

  while (content[i]) {
    size_t start = i;

    while (content[i] && content[i] != '\n') {
      ++i;
    }
    size_t line_len = i - start;
    if (content[i] == '\n') {
      ++i;
    }

    if (line_len == 0 && acc.size == 0) {
      continue;
    }

    char *line = strndup(content + start, line_len);
    if (!line) {
      break;
    }

    if (acc.size > 0) {
      sb_append_char(&acc, '\n');
    }
    sb_append(&acc, line);
    free(line);

    AstNode *root = NULL;
    StatusCode status = parse(sb_as_cstr(&acc), &root);

    if (status == INCOMPLETE_INPUT) {
      continue;
    }

    sb_free(&acc);
    acc = (StringBuffer){0};

    if (status != OK || !root) {
      fprintf(stderr, "42sh: %s: syntax error\n", name);
      shell->status = 2;
      continue;
    }

    shell->command = root;
    execute_command(root, shell);
    ast_free(root);
    shell->command = NULL;
  }

  if (acc.size > 0) {
    fprintf(stderr, "42sh: %s: unexpected end of file\n", name);
    sb_free(&acc);
    shell->status = 2;
  }

  free(content);
  return shell->status;
}

static int run_file(const char *path, Shell *shell) {
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "42sh: %s: No such file or directory\n", path);
    return 127;
  }
  int status = run_fd(fd, path, shell);
  close(fd);
  return status;
}

static AstNode *get_command(void) {
  StringBuffer sb = {0};
  char *line = readline("$ ");
  if (!line) {
    exit(EXIT_SUCCESS);
  }

  sb_append(&sb, line);
  free(line);

  while (true) {
    if (strendswith(sb_as_cstr(&sb), "\\")) {
      line = readline("> ");
      if (!line) {
        sb_free(&sb);
        exit(EXIT_SUCCESS);
      }
      sb_append(&sb, line);
      free(line);
      continue;
    }

    AstNode *root = NULL;
    StatusCode status = parse(sb_as_cstr(&sb), &root);
    if (status == INCOMPLETE_INPUT) {
      line = readline("> ");
      if (!line) {
        sb_free(&sb);
        exit(EXIT_SUCCESS);
      }
      sb_append(&sb, line);
      free(line);
      continue;
    }

    if (status != OK) {
      sb_free(&sb);
      return NULL;
    }

    sb_free(&sb);
    return root;
  }
}

static void run_interactive(Shell *shell) {
  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = SIG_IGN;
  sigaction(SIGQUIT, &sa, NULL);

  while (true) {
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sa, NULL);

    shell->command = get_command();
    if (!shell->command) {
      continue;
    }
    
    sa.sa_handler = SIG_IGN;
    sigaction(SIGINT, &sa, NULL);
    
    execute_command(shell->command, shell);
    ast_free(shell->command);
    shell->command = NULL;
  }
}

static bool opt_has(const char *arg, char opt) {
  if (!arg || arg[0] != '-' || arg[1] == '-' || arg[1] == '\0') {
    return false;
  } else {
    return arg[1] == opt;
  }
}

int main(int argc, char **argv, char **envp) {
  Shell shell = {
    .environment = (HashTable){
      .buckets = NULL,
      .size = 0,
      .capacity = 0,
      .free = env_variable_free
    },
    .aliases = (HashTable){
      .buckets = NULL,
      .size = 0,
      .capacity = 0,
      .free = free
    },
    .status = 0,
  };

  env_from_cstr_array(&shell.environment, (const char **)envp);

  --argc;
  ++argv;

  while (argc > 0 && (*argv)[0] == '-') {
    if (strcmp(*argv, "--") == 0) {
      --argc;
      ++argv;
      break;
    }

    if (opt_has(*argv, 'c')) {
      --argc;
      ++argv;
      if (argc == 0) {
        fprintf(stderr, "42sh: -c: option requires an argument\n");
        ht_clear(&shell.environment);
        ht_clear(&shell.aliases);
        return 2;
      }
      run_string(*argv, &shell);
      ht_clear(&shell.environment);
      ht_clear(&shell.aliases);
      return shell.status;
    }

    fprintf(stderr, "42sh: %s: invalid option\n", *argv);
    ht_clear(&shell.environment);
    ht_clear(&shell.aliases);
    return 2;
  }

  if (argc > 0) {
    run_file(*argv, &shell);
  } else if (isatty(STDIN_FILENO)) {
    shell.interactive = true;
    run_interactive(&shell);
  } else {
    run_fd(STDIN_FILENO, "stdin", &shell);
  }

  ht_clear(&shell.environment);
  ht_clear(&shell.aliases);
  return shell.status;
}