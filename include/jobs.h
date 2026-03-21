#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>

typedef enum {
  JOB_RUNNING,
  JOB_STOPPED,
  JOB_DONE,
} JobStatus;

typedef struct {
  pid_t *data;
  size_t size;
  size_t capacity;
} Pids;

typedef struct {
  int            id;
  pid_t          pgid;
  Pids           pids;
  JobStatus      status;
  int            exit_status;
  char          *command;
  bool           notified;
  struct termios tmodes;
} Job;

typedef struct {
  Job    *data;
  size_t  size;
  size_t  capacity;
} Jobs;

Job *job_add(Jobs *jobs, pid_t pgid, pid_t first_pid, const char *command);
void job_add_pid(Job *job, pid_t pid);
Job *job_find_pgid(Jobs *jobs, pid_t pgid);
Job *job_find_id(Jobs *jobs, int id);
Job *job_current(Jobs *jobs);
void job_remove(Jobs *jobs, Job *job);
void jobs_notify(Jobs *jobs);
void jobs_cleanup(Jobs *jobs);
