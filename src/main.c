#include "lexer.h"
#include "parser.h"
#include "sb.h"
#include "vec.h"

#include <readline/history.h>
#include <readline/readline.h>

AstNode *get_statement() {
  StringBuffer sb = {0};
  char *line = readline("$ ");
  if (!line) {
    return NULL;
  }

  sb_append(&sb, line);

  AstNode *root = NULL;
  while (line) {
    Tokens tokens = (Tokens){0};
    if (!tokenize(sb_as_cstr(&sb), &tokens)) {
      free(line);
      line = readline("> ");
      sb_append(&sb, line);
      for (size_t i = 0; i < vec_size(&tokens); ++i) {
        Token *token = &vec_at(&tokens, i);
        if (token->s) {
          free(token->s);
        }
      }
      vec_free(&tokens);
      continue;
    }

    if (!parse(&tokens, &root)) {
      free(line);
      line = readline("> ");
      sb_append(&sb, line);
      vec_free(&tokens);
      ast_free(root);
      root = NULL;
    } else {
      add_history(sb_as_cstr(&sb));
      vec_free(&tokens);
      sb_free(&sb);
      free(line);
      break;
    }
  }

  return root;
}

int main(int argc, char **argv, char **envp) {
  (void)argc;
  (void)argv;
  (void)envp;

  using_history();

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
