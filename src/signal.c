#include <unistd.h>

void sigint_handler(int sig) {
  (void)sig;

  write(STDOUT_FILENO, "\n", 1);
}

void sigint_heredoc_handler(int sig) {
  (void)sig;

  write(STDOUT_FILENO, "\n", 1);
}
