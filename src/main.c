#include "input.h"
#include "lexer.h"
#include "parser.h"
#include "vec.h"
#include <stddef.h>

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
      for (size_t i = 0; i < vec_size(&tokens); ++i) {
        if (tokens.data[i].type == TOKEN_WORD) {
          free(tokens.data[i].s);
        }
      }
      vec_free(&tokens);
      continue;
    }

    vec_free(&tokens);
    ast_print(root);
    ast_free(&root);
  }
  return 0;
}
