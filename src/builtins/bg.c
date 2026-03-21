#include "42sh.h"
#include "ast.h"
#include "vec.h"
#include "jobs.h"

#include <stdio.h>
#include <signal.h>

void builtin_bg(AstNode *node) {
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
    fprintf(stderr, "%s: bg: no current job\n", program_name);
    return;
  }
  
  if (job->status != JOB_STOPPED) {
    fprintf(stderr, "%s: bg: job already running\n", program_name);
    return;
  }

  job->status = JOB_RUNNING;
  killpg(job->pgid, SIGCONT);
  fprintf(stderr, "[%d]+ %s &\n", job->id, job->command);
}
