#include "42sh.h"
#include "status.h"
#include "sb.h"

#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

AstNode *get_command() {
  StringBuffer sb = {0};
  char *line = readline("$ ");
  if (!line) {
    exit(EXIT_SUCCESS);
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
        sb_append_char(&sb, '\n');
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

  Environment env = {0};
  env_copy(&env, (const char **)envp);

  using_history();

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

    execute_command(root, &env);

    ast_free(root);
  }

  env_free(&env);

  clear_history();

  return 0;
}
