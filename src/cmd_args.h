#ifndef CMD_ARGS_H
#define CMD_ARGS_H

#include "idl.h"

typedef struct cmd_arg_opt_t {
    char *key;
    char *value;
    struct cmd_arg_opt_t *next;
} cmd_arg_opt_t;

typedef struct {
    cmd_arg_opt_t *head;
} cmd_args_t;

void cmd_args_parse(cmd_args_t *args, int argc, char *argv[], int start_index);
const char* cmd_args_get_value(cmd_args_t *args, const char *key);
bool cmd_args_has_flag(cmd_args_t *args, const char *key);
void cmd_args_clear(cmd_args_t *args);

#endif // CMD_ARGS_H