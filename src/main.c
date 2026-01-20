#include "input.h"
#include "lexer.h"
#include "parser.h"
#include "vec.h"

int main() {
  while (true) {
    Tokens tokens = {0};
    char *input = read_input();
    if (!input) {
      continue;
    } else if (!tokenize(input, &tokens)) {
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
  return 0;
}
