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
  free(line);

  AstNode *root = NULL;
  Tokens tokens = {0};
  while (true) {
    if (!tokenize(sb_as_cstr(&sb), &tokens)) {
      for (size_t i = 0; i < vec_size(&tokens); ++i) {
        Token *token = &vec_at(&tokens, i);
        if (token->s) {
          free(token->s);
        }
      }
      vec_free(&tokens);

      line = readline("> ");
      if (!line) {
        break;
      }
      sb_append(&sb, line);
      free(line);
      continue;
    }

    if (!parse(&tokens, &root)) {
      vec_free(&tokens);
      ast_free(root);
      line = readline("> ");
      if (!line) {
        break;
      }
      sb_append(&sb, line);
      free(line);
      continue;
    }

    add_history(sb_as_cstr(&sb));
    vec_free(&tokens);
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
