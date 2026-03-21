#include "jobs.h"
#include "42sh.h"
#include "vec.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

Job *job_add(Jobs *jobs, pid_t pgid, pid_t first_pid, const char *command) {
  int id = 1;
  bool taken = true;
  while (taken) {
    taken = false;
    for (size_t i = 0; i < jobs->size; ++i) {
      if (jobs->data[i].id == id) {
        id++;
        taken = true;
        break;
      }
    }
  }

  Job j = {0};
  j.id      = id;
  j.pgid    = pgid;
  j.command = strdup(command);
  j.status  = JOB_RUNNING;
  vec_push(&j.pids, first_pid);

  vec_push(jobs, j);

  return &vec_last(jobs);
}

void job_add_pid(Job *job, pid_t pid) {
  vec_push(&job->pids, pid);
}

Job *job_find_pgid(Jobs *jobs, pid_t pgid) {
  vec_foreach(Job, job, jobs) {
    if (job->pgid == pgid) {
      return job;
    }
  }
  
  return NULL;
}

Job *job_find_id(Jobs *jobs, int id) {
  vec_foreach(Job, job, jobs) {
    if (job->id == id) {
      return job;
    }
  }
  
  return NULL;
}

Job *job_current(Jobs *jobs) {
  vec_foreach(Job, job, jobs) {
    if (job->status != JOB_DONE) {
      return job;
    }
  }
  
  return NULL;
}

void job_remove(Jobs *jobs, Job *job) {
  free(job->command);
  vec_free(&job->pids);
  size_t idx = job - jobs->data;
  memmove(&jobs->data[idx], &jobs->data[idx + 1], (jobs->size - idx - 1) * sizeof(Job));
  jobs->size--;
}

void jobs_notify(Jobs *jobs) {
  size_t i = 0;
  while (i < jobs->size) {
    Job *job = &jobs->data[i];
    if (job->status == JOB_DONE && !job->notified) {
      fprintf(stderr, "[%d]+  Done\t\t%s\n", job->id, job->command);
      job_remove(jobs, job);
    } else if (job->status == JOB_STOPPED && !job->notified) {
      fprintf(stderr, "[%d]+  Stopped\t\t%s\n", job->id, job->command);
      job->notified = true;
      ++i;
    } else {
      ++i;
    }
  }
}

void jobs_cleanup(Jobs *jobs) {
  for (size_t i = 0; i < jobs->size; ++i) {
    free(jobs->data[i].command);
    vec_free(&jobs->data[i].pids);
  }
  free(jobs->data);
  jobs->data = NULL;
  jobs->size = 0;
  jobs->capacity = 0;
}
