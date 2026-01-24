#include "42sh.h"
#include "error.h"
#include "lexer.h"
#include "parser.h"
#include "sb.h"

#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

AstNode *get_statement() {
  StringBuffer sb = {0};
  char *line = readline("$ ");
  if (!line) {
    exit(EXIT_SUCCESS);
    return NULL;
  }

  sb_append(&sb, line);
  free(line);

  while (true) {
    StatusCode status;
    Tokens tokens = {0};
    AstNode *root = NULL;

    status = lex(sb_as_cstr(&sb), &tokens);
    if (status != OK) {
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
      break;
    }

    status = parse(&tokens, &root);
    tokens_free(&tokens);
    if (status != OK) {
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
      break;
    }

    add_history(sb_as_cstr(&sb));
    sb_free(&sb);
    return root;
  }

  sb_free(&sb);
  return NULL;
}

int main(int argc, char **argv, char **envp) {
  (void)argc;
  (void)argv;
  (void)envp;

  using_history();

  struct sigaction sa;
  sa.sa_handler = sigint_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);

  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGQUIT, &sa, NULL);

  while (true) {
    AstNode *root = get_statement();
    if (!root) {
      continue;
    }

    ast_print(root);
    ast_free(root);
    root = NULL;
  }

  clear_history();
  return 0;
}
