#ifndef IDL_PROCESS_RUNNER_H
#define IDL_PROCESS_RUNNER_H

#include "idl.h"
#include "compiler_command.h"

typedef struct {
    const char *label;          // Texto exibido ao iniciar o job (NULL = nada)
    compiler_command_t *cmd;    // argv terminado em NULL; args[0] é o executável
    bool capture;               // true: guarda o stdout em output e não exibe nada
    char *output;               // stdout capturado (liberar com free)
    int64 exit_status;          // Preenchido após a execução
} process_job_t;

/* Executa os jobs em paralelo, até max_parallel ao mesmo tempo (0 = número de núcleos).
   Retorna true se todos terminaram com código 0. */
bool process_runner_run(process_job_t *jobs, word count, uint32 max_parallel);

#endif // IDL_PROCESS_RUNNER_H
