#include "42sh.h"
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "sb.h"
#include "status.h"

#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

bool is_interactive = false;

bool strendswith(const char *s, const char *suffix) {
  if (!s || !suffix) {
    return false;
  }
  
  size_t s_len = strlen(s);
  size_t suffix_len = strlen(suffix);
  
  if (suffix_len > s_len) {
    return false;
  }

  return strcmp(s + s_len - suffix_len, suffix) == 0;
}

AstNode *get_command() {
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

    if (has_heredocs(root)) {
      read_heredocs(&sb, root);
    }

    sb_free(&sb);
    return root;
  }

  sb_free(&sb);
  return NULL;
}

int main(int argc, char **argv, char **envp) {
  (void)argc;
  (void)argv;

  Shell shell = {0};

  env_from_cstr_array(&shell.environment, (const char **)envp);

  if (isatty(STDIN_FILENO)) {
    is_interactive = true;
  }

  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = SIG_IGN;
  sigaction(SIGQUIT, &sa, NULL);

  while (true) {
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sa, NULL);

    AstNode *root = get_command();
    if (!root) {
      continue;
    }

    sa.sa_handler = SIG_IGN;
    sigaction(SIGINT, &sa, NULL);

    execute_command(root, &shell);

    ast_free(root);
  }

  env_free(&shell.environment);
  env_free(&shell.local);
  env_free(&shell.aliases);

  return 0;
}
