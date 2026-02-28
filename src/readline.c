#include "sb.h"
#include "shell.h"

#include <stdbool.h>
#include <unistd.h>

char *readline_interactive(Shell *shell) {
  const char *prompt = shell->input.size == 0 ? "$ " : "> ";
  write(STDERR_FILENO, prompt, strlen(prompt));

  StringBuffer sb = {0};
  while (true) {
    char c = '\0';
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

char *readline_string(Shell *shell) {
  const char *s = shell->input_src;
  if (!s || *s == '\0')
    return NULL;

  const char *end = strchr(s, '\n');
  char *line;
  if (end) {
    line = strndup(s, end - s + 1);
    shell->input_src = end + 1;
  } else {
    line = strdup(s);
    shell->input_src = s + strlen(s);
  }
  return line;
}

char *readline_fd(Shell *shell) {
  StringBuffer sb = {0};
  char c;

  while (true) {
    ssize_t n = read(shell->input_fd, &c, 1);
    if (n <= 0) {
      if (sb.size > 0)
        break;
      sb_free(&sb);
      return NULL;
    }
    sb_append_char(&sb, c);
    if (c == '\n')
      break;
  }

  return sb_as_cstr(&sb);
}
