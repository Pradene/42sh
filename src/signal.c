#include <readline/readline.h>
#include <unistd.h>

void sigint_handler(int sig) {
  (void)sig;

  write(STDOUT_FILENO, "\n", 1);
  rl_on_new_line();
  rl_replace_line("", 0);
  rl_redisplay();
}

void sigint_heredoc_handler(int sig) { (void)sig; }
