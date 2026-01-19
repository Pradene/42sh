#include "input.h"
#include "lexer.h"
#include "vec.h"
#include <stdio.h>

int main() {
  while (true) {
    Tokens tokens = {0};
    char *input = read_input();
    if (!input || !tokenize(input, &tokens)) {
      continue;
    }
    for (size_t i = 0; i < tokens.size; ++i) {
      printf("%s\n", token_type_str(tokens.data[i].type));
      if (tokens.data[i].type == TOKEN_WORD) {
        printf("%s\n", tokens.data[i].s);
        free(tokens.data[i].s);
      }
    }
    vec_free(&tokens);
    free(input);
  }
  return 0;
}
