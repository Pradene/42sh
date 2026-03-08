#include "42sh.h"
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "sb.h"
#include "status.h"
#include "utils.h"

#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

HashTable *environ = NULL;
HashTable *aliases = NULL;

uint8_t exit_status = 0;

bool is_interactive = false;
bool is_continuation = false;

AstNode *last_command = NULL;

static bool opt_has(const char *arg, char opt) {
  if (!arg || arg[0] != '-' || arg[1] == '-' || arg[1] == '\0') {
    return false;
  } else {
    return arg[1] == opt;
  }
}

static void run(Shell *shell, char *(*readline)(Shell *shell)) {
  StringBuffer sb = {0};
  
  while (true) {
    AstNode *command = NULL;

    char *line = readline(shell);
    if (!line) {
      break;
    }

    sb_append(&sb, line);
    free(line);

    if (strendswith(sb_as_cstr(&sb), "\\\n")) {
      is_continuation = true;
      continue;
    }

    char *expanded = expand_alias(sb_as_cstr(&sb));

    StatusCode status = parse((const char *)expanded, &command);
    free(expanded);
    if (status == INCOMPLETE_INPUT) {
      is_continuation = true;
      continue;
    }

    is_continuation = false;
    sb_free(&sb);

    if (status != OK) {
      continue;
    }

    last_command = command;

    expand_command(command, shell);
    execute_command(command, shell);

    ast_free(command);
  }
}

int main(int argc, char **argv, char **envp) {
  Shell shell = {0};

  environ = ht_with_capacity(32);
  environ->free = env_variable_free;

  aliases = ht_with_capacity(32);
  aliases->free = free;

  environ_from_envp(environ, (const char **)envp);

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
        shell_destroy(&shell);
        return 2;
      }
      shell.input_src = *argv;
      run(&shell, readline_string);
      shell_destroy(&shell);
      return exit_status;
    }

    fprintf(stderr, "42sh: %s: invalid option\n", *argv);
    shell_destroy(&shell);
    return 2;
  }

  if (argc > 0) {
    int fd = open(*argv, O_RDONLY);
    if (fd < 0) {
      fprintf(stderr, "42sh: %s: cannot open file\n", *argv);
      shell_destroy(&shell);
      return 2;
    }
    shell.input_fd = fd;
    run(&shell, readline_fd);
    close(fd);
  } else if (isatty(STDIN_FILENO)) {
    is_interactive = true;
    run(&shell, readline_interactive);
  } else {
    shell.input_fd = STDIN_FILENO;
    run(&shell, readline_fd);
  }

  shell_destroy(&shell);
  return exit_status;
}
