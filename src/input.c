#include "strbuf.h"
#include <readline/readline.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

char *read_input() {
  StringBuffer sb = {0};
  char *line = readline("$ ");
  if (!line) {
    return NULL;
  }

  while (line) {
    size_t length = strlen(line);
    bool newline_escaped = false;

    if (length > 0 && line[length - 1] == '\\') {
      line[length - 1] = '\0';
      newline_escaped = true;
    }

    sb_append(&sb, line);
    free(line);

    if (newline_escaped) {
      line = readline("> ");
      continue;
    }

    break;
  }

  sb_appendc(&sb, '\0');
  return sb_as_cstr(&sb);
}
