#include "input.h"
#include "lexer.h"
#include "parser.h"
#include "vec.h"
#include <readline/history.h>

int main(int argc, char **argv, char **envp) {
  (void)argc;
  (void)argv;
  (void)envp;

  using_history();

  while (true) {
    Tokens tokens = {0};
    char *input = read_input();
    if (!input) {
      continue;
    }

    add_history(input);

    if (!tokenize(input, &tokens)) {
      free(input);
      continue;
    }
    free(input);

    AstNode *root = NULL;
    if (!parse(&tokens, &root)) {
      vec_free(&tokens);
      continue;
    }

    vec_free(&tokens);
    ast_print(root);
    ast_free(&root);
  }

  clear_history();
  return 0;
}
