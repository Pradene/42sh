#include "42sh.h"

#include <unistd.h>

void sigint_handler(int sig) {
  (void)sig;
  write(STDOUT_FILENO, "\n", 1);
}
