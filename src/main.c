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

static bool opt_has(const char *arg, char opt) {
  if (!arg || arg[0] != '-' || arg[1] == '-' || arg[1] == '\0') {
    return false;
  } else {
    return arg[1] == opt;
  }
}

static int run(Shell *shell) {
  while (true) {
    shell->command = NULL;

    char *line = shell->readline(shell);
    if (!line) {
      break;
    }

    sb_append(&shell->input, line);
    free(line);

    if (strendswith(sb_as_cstr(&shell->input), "\\")) {
      continue;
    }

    expand_alias(shell);

    StatusCode status = parse(shell);
    if (status == INCOMPLETE_INPUT) {
      continue;
    }

    if (status != OK) {
      sb_free(&shell->input);
      if (shell->interactive) {
        continue;
      } else {
        break;
      }
    }

    expand_command(shell);
    execute_command(shell);

    ast_free(shell->command);
    sb_free(&shell->input);
  }

  return shell->status;
}

int main(int argc, char **argv, char **envp) {
  Shell shell = shell_create();

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
        shell_destroy(&shell);
        return 2;
      }
      // run_string(*argv, &shell);
      shell_destroy(&shell);
      return shell.status;
    }

    fprintf(stderr, "42sh: %s: invalid option\n", *argv);
    shell_destroy(&shell);
    return 2;
  }

  if (argc > 0) {
    // run_file(*argv, &shell);
  } else if (isatty(STDIN_FILENO)) {
    shell.interactive = true;
    shell.readline = readline;
    run(&shell);
  } else {
    // run_fd(STDIN_FILENO, "stdin", &shell);
  }


  shell_destroy(&shell);
  return shell.status;
}
