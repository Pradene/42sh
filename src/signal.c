#include "42sh.h"

#include <unistd.h>
#include <signal.h>

void signals_reset(void) {
  signal(SIGINT, SIG_DFL);
  signal(SIGQUIT, SIG_DFL);
  signal(SIGTSTP, SIG_DFL);
  signal(SIGTTIN, SIG_DFL);
  signal(SIGTTOU, SIG_DFL);
}

void sigint_handler(int sig) {
  (void)sig;
  write(STDOUT_FILENO, "\n", 1);
}
