#include "idl.h"
#include "cmd_command.h"

int handle_param_help(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    printf("A C/C++ project manager: builds projects that follow a directory convention,\n"
           "without Makefiles or CMake.\n"
           "\n"
           "Usage: idl <COMMAND> [OPTIONS]\n"
           "\n");

    idl_print_commands();

    printf("\n"
           "Options (build, run, test, clean):\n"
           "      --project <DIR>  Run the command in the given project directory\n"
           "      --release        Use the release profile (build/release/)\n"
           "      --bin <NAME>     run: choose the executable from src/bin/\n"
           "      -- <ARGS>...     run: arguments passed to the program\n"
           "  -h, --help           Display this help\n"
           "\n"
           "Environment:\n"
           "  CC, CXX, AR          Compilers and archiver to use (default: gcc/g++, then clang/clang++)\n"
           "  IDL_DEBUG=1          Show debug messages\n");

    return 0;
}
