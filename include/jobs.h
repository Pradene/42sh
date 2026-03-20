#pragma once

#include <stdint.h>

typedef enum {
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE,
} JobStatus;

typedef struct {
    uint32_t  jobspec;
    uint32_t  pgid;
    uint32_t  exit_status;
    char     *command;
    JobStatus status;
} Job;

typedef struct {
    Job    *data;
    size_t  size;
    size_t  capacity;
} Jobs;