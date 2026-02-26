#include "sb.h"

#include <stdbool.h>
#include <unistd.h>

char *readline(const char *prompt) {
  StringBuffer sb = {0};
  char c;

  if (prompt) {
    write(STDERR_FILENO, prompt, strlen(prompt));
  }

  while (true) {
    int readc = read(STDIN_FILENO, &c, 1);
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
