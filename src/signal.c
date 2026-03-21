#include "42sh.h"
#include "vec.h"

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

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

void sigchld_handler(int sig) {
  (void)sig;

  int   wstatus;
  pid_t pid;

  while ((pid = waitpid(-1, &wstatus, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
    Job *job = NULL;
    for (size_t i = 0; i < jobs.size; i++) {
      vec_foreach(pid_t, p, &jobs.data[i].pids) {
        if (*p == pid) {
          job = &jobs.data[i];
          break;
        }
      }

      if (job) {
        break;
      }
    }

    if (!job) {
      continue;
    }

    if (WIFEXITED(wstatus) || WIFSIGNALED(wstatus)) {
      job->exit_status = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : 128 + WTERMSIG(wstatus);
      job->status      = JOB_DONE;
      job->notified    = false;
    } else if (WIFSTOPPED(wstatus)) {
      tcgetattr(STDIN_FILENO, &job->tmodes);
      job->status   = JOB_STOPPED;
      job->notified = false;
    } else if (WIFCONTINUED(wstatus)) {
      job->status   = JOB_RUNNING;
      job->notified = true;
    }
  }
}