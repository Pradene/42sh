#include "42sh.h"
#include "ast.h"
#include "vec.h"

#include <stdio.h>

void builtin_jobs(AstNode *node) {
  if (vec_size(&node->command.args) == 1) {
    vec_foreach(Job, job, &jobs) {
      const char *status_str =
        job->status == JOB_RUNNING ? "Running" :
        job->status == JOB_STOPPED ? "Stopped" : "Done";
      printf("[%d]%s  %-10s %s\n",
        job->id,
        " ",
        status_str,
        job->command
      );
    }
  } else {}
}
