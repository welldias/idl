#include "idl.h"
#include "cmd_command.h"

static void help_print_general(void) {
    printf("A C/C++ project manager: builds projects that follow a directory convention,\n"
           "without Makefiles or CMake.\n"
           "\n"
           "Usage: idl <COMMAND> [ARGS]\n"
           "\n");

    idl_print_commands();

    printf("\n"
           "Run 'idl help <COMMAND>' to see how to use a command.\n"
           "\n"
           "Environment:\n"
           "  CC, CXX, AR  Compilers and archiver to use (default: gcc/g++, then clang/clang++)\n"
           "  IDL_DEBUG=1  Show debug messages\n");
}

/* idl help [<command>] */
int handle_param_help(int argc, char *argv[]) {
    if (argc <= 2) {
        help_print_general();
        return 0;
    }

    if (argc > 3) {
        log_error("Unexpected argument '%s'. Usage: idl help [COMMAND]", argv[3]);
        return 1;
    }

    const command_t *command = idl_find_command(argv[2]);
    if (!command) {
        log_error("Unknown command '%s'. Run 'idl help' to list the commands.", argv[2]);
        return 1;
    }

    printf("idl %s: %s\n\n", command->name, command->description);
    if (command->usage)
        fputs(command->usage, stdout);
    else
        printf("Not implemented yet.\n");
    return 0;
}
