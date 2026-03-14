#include "42sh.h"
#include "ast.h"
#include "parser.h"
#include "sb.h"
#include "status.h"
#include "utils.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

HashTable *environ = NULL;
HashTable *aliases = NULL;

uint8_t exit_status = 0;

bool is_interactive = false;
bool is_continuation = false;

AstNode *last_command = NULL;

char *input_string = NULL;
int input_fd = STDIN_FILENO;

struct termios term_attr;

void cleanup() {
  tcsetattr(STDIN_FILENO, TCSANOW, &term_attr);
  ht_destroy(environ);
  ht_destroy(aliases);
  ast_free(last_command);
}

static bool opt_has(const char *arg, char opt) {
  if (!arg || arg[0] != '-' || arg[1] == '-' || arg[1] == '\0') {
    return false;
  } else {
    return arg[1] == opt;
  }
}

static void run(char *(*getline)()) {
  StringBuffer sb = {0};
  AstNode *command = NULL;

  while (true) {
    errno = 0;
    command = NULL;
    last_command = NULL;

    char *line = getline();
    if (!line) {
      if (is_interactive) {
        if (errno == EINTR) {
          if (is_continuation) {
            is_continuation = false;
            sb_free(&sb);
          }

          continue;
        } else if (errno == 0 && is_continuation) {
          is_continuation = false;
          sb_free(&sb);
          continue;
        } else {
          break;
        }
      } else {
        break;
      }
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

    expand_command(command);
    execute_command(command);

    ast_free(command);
  }
}

int main(int argc, char **argv, char **envp) {
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
        cleanup();
        return 2;
      }
      input_string = *argv;
      run(getline_from_string);
      cleanup();
      return exit_status;
    }

    fprintf(stderr, "42sh: %s: invalid option\n", *argv);
    cleanup();
    return 2;
  }

  if (argc > 0) {
    int fd = open(*argv, O_RDONLY);
    if (fd < 0) {
      fprintf(stderr, "42sh: %s: cannot open file\n", *argv);
      cleanup();
      return 2;
    }
    input_fd = fd;
    run(getline_from_fd);
    close(fd);
  } else if (isatty(STDIN_FILENO)) {
    is_interactive = true;
    input_fd = STDIN_FILENO;

    tcgetattr(STDIN_FILENO, &term_attr);

    struct termios raw = term_attr;
    raw.c_lflag &= ~ECHOCTL;

    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    struct sigaction sa = {0};
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sa, NULL);

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);

    run(getline_from_fd);
  } else {
    input_fd = STDIN_FILENO;
    run(getline_from_fd);
  }

  cleanup();
  return exit_status;
}
