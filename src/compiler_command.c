#include "compiler_command.h"

#define PARAMS_INIT_CAPACITY 10

void compiler_command_init(compiler_command_t *cmd, word capacity) {
    RETURN_IF_FAIL(cmd);

    cmd->capacity = capacity;
    cmd->count = 0;
    cmd->args = (char **)malloc(sizeof(char *) * cmd->capacity);
}

void compiler_commands_append(compiler_command_t *cmd, int count, char **args) {
    RETURN_IF_FAIL(cmd);
    RETURN_IF_FAIL(args);
    RETURN_IF_FAIL(count > 0);

    word new_capacity = cmd->count + count + 1;

    if (new_capacity > cmd->capacity) {
        char **new_args = (char **)realloc(cmd->args, sizeof(char *) * new_capacity);
        if (new_args == NULL) {
            return; // Out of memory
        }
        cmd->args = new_args;
        cmd->capacity = new_capacity;
    }

    for (int i = 0; i < count; i++) {
        cmd->args[cmd->count++] = args[i];
    }

    cmd->args[cmd->count] = NULL;
}

void compiler_command_clear(compiler_command_t *cmd) {
    RETURN_IF_FAIL(cmd);

    free(cmd->args);
    memset(cmd, 0, sizeof(compiler_command_t));
}
