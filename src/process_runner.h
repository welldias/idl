#ifndef IDL_PROCESS_RUNNER_H
#define IDL_PROCESS_RUNNER_H

#include "idl.h"
#include "compiler_command.h"

typedef struct {
    const char *label;          // Text printed when the job starts (NULL = nothing)
    compiler_command_t *cmd;    // NULL-terminated argv; args[0] is the executable
    bool capture;               // true: store stdout in output and print nothing
    char *output;               // captured stdout (release with free)
    int64 exit_status;          // Filled in after the run
} process_job_t;

/* Runs the jobs in parallel, up to max_parallel at a time (0 = number of cores).
   Returns true if all of them exited with code 0. */
bool process_runner_run(process_job_t *jobs, word count, uint32 max_parallel);

#endif // IDL_PROCESS_RUNNER_H
