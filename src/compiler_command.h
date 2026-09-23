#ifndef IDL_COMPILER_COMMAND_H
#define IDL_COMPILER_COMMAND_H

#include "idl.h"

typedef struct {
    char **args;
    word count;
    word capacity;
} compiler_command_t;

void compiler_command_init(compiler_command_t *cmd, word capacity);
void compiler_commands_append(compiler_command_t *cmd, int count, char **args);
void compiler_command_clear(compiler_command_t *cmd);


#define COMPILER_COMMANDS_APPEND(cmd, ...) \
    compiler_commands_append(cmd, sizeof((char*[]){__VA_ARGS__})/sizeof(char*), (char*[]){__VA_ARGS__})

#endif // IDL_COMPILE_COMMAND_H