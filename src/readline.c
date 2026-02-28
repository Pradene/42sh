#include "sb.h"
#include "shell.h"

#include <stdbool.h>
#include <unistd.h>

char *readline(Shell *shell) {
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
