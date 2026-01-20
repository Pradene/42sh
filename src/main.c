#include "input.h"
#include "lexer.h"
#include "parser.h"
#include "vec.h"

int main() {
  while (true) {
    Tokens tokens = {0};
    char *input = read_input();
    if (!input || !tokenize(input, &tokens)) {
      continue;
    }
    free(input);

    AstNode *root = NULL;
    if (!parse(&tokens, &root)) {
      continue;
    }
    vec_free(&tokens);
    ast_print(root);
  }
  return 0;
}
