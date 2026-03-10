#include "42sh.h"
#include "sb.h"

#include <stdbool.h>
#include <unistd.h>

char *getline_from_fd() {
  if (is_interactive) {
    const char *prompt = !is_continuation ? "$ " : "> ";
    write(STDERR_FILENO, prompt, strlen(prompt));
  }

  StringBuffer sb = {0};
  while (true) {
    char c = '\0';
    int readc = read(input_fd, &c, 1);
    if (readc <= 0) {
      break;
    }

    sb_append_char(&sb, c);
    if (c == '\n') {
      break;
    }
  }

  return sb_as_cstr(&sb);
}

char *getline_from_string() {
  char *s = input_string;
  if (!s || *s == '\0') {
    return NULL;
  }

  char *line;
  char *end = strchr(s, '\n');
  if (end) {
    line = strndup(s, end - s + 1);
    input_string = end + 1;
  } else {
    line = strdup(s);
    input_string = s + strlen(s);
  }
  return line;
}
