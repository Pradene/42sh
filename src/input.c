#include "strbuf.h"
#include <readline/readline.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static bool has_unclosed_pair(const char *s) {
  bool in_single = false;
  bool in_double = false;

  StringBuffer stack = {0};
  for (size_t i = 0; s && s[i]; ++i) {
    if (s[i] == '\\' && s[i + 1]) {
      ++i;
      continue;
    }

    if (s[i] == '\'' && !in_double) {
      in_single = !in_single;
      continue;
    } else if (s[i] == '\"' && !in_single) {
      in_double = !in_double;
      continue;
    }

    if (in_single || in_double) {
      continue;
    }

    if (s[i] == '(' || s[i] == '[' || s[i] == '{') {
      strbuf_appendc(&stack, s[i]);
    } else if (s[i] == ')' || s[i] == ']' || s[i] == '}') {
      if (stack.size == 0) {
        strbuf_free(&stack);
        return false;
      }

      char last = stack.data[stack.size - 1];
      bool match = (s[i] == ')' && last == '(') ||
                   (s[i] == ']' && last == '[') || (s[i] == '}' && last == '{');

      if (!match) {
        continue;
      }
      stack.size--;
    }
  }

  bool result = in_single || in_double || stack.size > 0;
  strbuf_free(&stack);
  return result;
}

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

    strbuf_append(&sb, line);
    free(line);

    if (newline_escaped) {
      line = readline("> ");
      continue;
    }

    if (has_unclosed_pair(strbuf_cstr(&sb))) {
      strbuf_appendc(&sb, '\n');
      line = readline("> ");
      continue;
    }

    break;
  }

  strbuf_appendc(&sb, '\0');
  return strbuf_cstr(&sb);
}
