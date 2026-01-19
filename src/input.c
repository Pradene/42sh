#include "strbuf.h"
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

char *read_input() {
  StringBuffer sb = {0};
  char *line = readline("$ ");
  if (!line)
    return NULL;

  while (line) {
    size_t length = strlen(line);
    bool newline_escaped = false;

    if (length > 0 && line[length - 1] == '\\') {
      line[length - 1] = '\0';
      newline_escaped = true;
    }

    strbuf_append(&sb, line);
    free(line);

    if (newline_escaped) {
      line = readline("> ");
      continue;
    }

    if (has_unfinished_quote(sb.data)) {
      strbuf_push(&sb, '\n');
      line = readline("> ");
      continue;
    }

    break;
  }

  strbuf_push(&sb, '\0');
  return sb.data;
}
