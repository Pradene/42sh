#include "vec.h"
#include <readline/readline.h>
#include <stdbool.h>
#include <stdlib.h>

static bool has_unfinished_quote(const char *s) {
  bool in_single = false;
  bool in_double = false;

  for (size_t i = 0; s[i]; ++i) {
    if (s[i] == '\\' && s[i + 1]) {
      ++i;
      continue;
    }
    if (s[i] == '\'' && !in_double) {
      in_single = !in_single;
    } else if (s[i] == '\"' && !in_single) {
      in_double = !in_double;
    }
  }

  return in_single || in_double;
}

typedef VEC(char) StringBuilder;
char *read_input() {
  StringBuilder sb = {0};
  char *line = readline("$ ");
  if (!line)
    return NULL;

  while (line) {
    size_t len = strlen(line);

    if (len > 0 && line[len - 1] == '\\') {
      for (size_t i = 0; i < len - 1; ++i) {
        vec_push(&sb, line[i]);
      }
      free(line);
      line = readline("> ");
      continue;
    }

    for (size_t i = 0; i < len; ++i) {
      vec_push(&sb, line[i]);
    }
    free(line);

    if (has_unfinished_quote(sb.data)) {
      vec_push(&sb, '\n');
      line = readline("> ");
      continue;
    }

    break;
  }

  vec_push(&sb, '\0');
  return sb.data;
}
