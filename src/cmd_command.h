#ifndef IDL_COMMAND_H
#define IDL_COMMAND_H

#include "idl.h"

typedef int (*command_handler_func)(int argc, char *argv[]);

typedef struct {
    const char *name;
    const char *description;
    command_handler_func handler;
    const char *usage;              // shown by "idl help <name>"; NULL: not implemented yet
} command_t;

int idl_execute_command(int argc, char *argv[]);
void idl_print_commands(void);
const command_t *idl_find_command(const char *name);

#endif // BX_COMMAND_H