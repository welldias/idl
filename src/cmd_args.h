#ifndef CMD_ARGS_H
#define CMD_ARGS_H

#include "idl.h"

typedef struct cmd_arg_opt_t {
    char *key;
    char *value;
    struct cmd_arg_opt_t *next;
} cmd_arg_opt_t;

/* The options a command accepts. Lists end with NULL; a NULL list means none. */
typedef struct {
    const char *const *with_value;  // options followed by a value: --project <dir> (or --project=<dir>)
    const char *const *flags;       // options without a value: --verbose
    int max_positionals;            // arguments that are not options; -1: any number
} cmd_args_spec_t;

typedef struct {
    cmd_arg_opt_t *head;
    list_t positionals;             // the arguments that are not options, in order (strings of argv)
} cmd_args_t;

/* Reads argv[start_index..argc). Only the options of <spec> are known, so a loose word is
   never taken for the value of a flag. Returns false (with a message) on an unknown option,
   a missing value or too many arguments. */
bool cmd_args_parse(cmd_args_t *args, int argc, char *argv[], int start_index, const cmd_args_spec_t *spec);
const char *cmd_args_get_value(cmd_args_t *args, const char *key);
bool cmd_args_has_flag(cmd_args_t *args, const char *key);
void cmd_args_clear(cmd_args_t *args);

#endif // CMD_ARGS_H
