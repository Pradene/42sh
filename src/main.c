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

#include <readline/readline.h>
#include <readline/history.h>

AstNode *get_command() {
  StringBuffer sb = {0};
  char *line = readline("$ ");
  if (!line) {
    exit(EXIT_SUCCESS);
  }

  sb_append(&sb, line);
  free(line);

  while (true) {
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

  rl_getc_function = fgetc;

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

  return 0;
}
