#include "42sh.h"
#include "ast.h"
#include "vec.h"
#include "jobs.h"

#include <stdio.h>
#include <sys/wait.h>

void builtin_fg(AstNode *node) {
  Job *job = NULL;

  if (node && vec_size(&node->command.args) >= 2) {
    const char *arg = node->command.args.data[1];
    if (arg[0] == '%') {
      int id = atoi(arg + 1);
      job = job_find_id(&jobs, id);
    } else {
      int id = atoi(arg);
      job = job_find_id(&jobs, id);
    }
  } else {
    job = job_current(&jobs);
  }

  if (!job) {
    fprintf(stderr, "%s: fg: no current job\n", program_name);
    return;
  }

  fprintf(stderr, "%s\n", job->command);
  job->status = JOB_RUNNING;

  killpg(job->pgid, SIGCONT);

  signal(SIGTTOU, SIG_IGN);
  tcsetpgrp(STDIN_FILENO, job->pgid);

  int status;
  waitpid(-job->pgid, &status, WUNTRACED);

  tcsetpgrp(STDIN_FILENO, getpgrp());
  signal(SIGTTOU, SIG_DFL);

  if (WIFSTOPPED(status)) {
    job->status = JOB_STOPPED;
    fprintf(stderr, "\n[%d]+  Stopped\t%s\n", job->id, job->command);
  } else {
    exit_status = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
    job_remove(&jobs, job);
  }
}
