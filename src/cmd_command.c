#include "cmd_handlers.h"
#include "cmd_command.h"

#define USAGE_PROJECT "  -p, --project <DIR>  Run the command in the given project directory\n"

const command_t command_list[] = {
    { "run", "Build and run the project's executable", handle_param_run,
      "Usage: idl run [EXECUTABLE] [OPTIONS] [-- ARGS...]\n"
      "\n"
      "Builds the executable (debug profile) and what it links, then runs it.\n"
      "Without a name: the only executable, or the one named after the project.\n"
      "The arguments after -- are passed to the program; its exit code is idl's.\n"
      "\n"
      "Options:\n"
      USAGE_PROJECT },
    { "init", "Create a new project", handle_param_init,
      "Usage: idl init\n"
      "\n"
      "Creates project.yml, README.md and src/main.c in the current directory\n"
      "(existing files are kept). The project is named after the directory.\n" },
    { "add", "Add dependencies to the project", handle_param_add,
      "Usage: idl add <DEPENDENCY>...\n"
      "\n"
      "Looks for each library installed on the machine and adds it to\n"
      "project.dependencies, in this order:\n"
      "  1. the known ones: pthread, m, dl, rt\n"
      "  2. pkg-config\n"
      "  3. the directories of LD_LIBRARY_PATH\n"
      "  4. the system library directories (/usr/lib, /usr/local/lib...)\n"
      "The name is that of the library: foo for libfoo.so (libfoo also works).\n"
      "If one of them is not found, nothing is saved. The envs: section of\n"
      "project.yml counts for the search (e.g. PKG_CONFIG_PATH).\n" },
    { "remove", "Remove dependencies from the project", handle_param_remove, nullptr },
    { "version", "Read or update the project's version", handle_param_version, nullptr },
    { "sync", "Update the project's environment", handle_param_sync, nullptr },
    { "lock", "Update the project's lockfile", handle_param_lock, nullptr },
    { "export", "Export the project's lockfile to an alternate format", handle_param_export, nullptr },
    { "tree", "Display the project's dependency tree", handle_param_tree, nullptr },
    { "tool", "Run and install commands provided by C packages", handle_param_tool, nullptr },
    { "c", "Manage C versions and installations", handle_param_c, nullptr },
    { "venv", "Create a virtual environment", handle_param_venv, nullptr },
    { "build", "Build the project (debug)", handle_param_build,
      "Usage: idl build [TARGET]... [OPTIONS]\n"
      "\n"
      "Builds the project with the debug profile (-g -O0) into build/debug/.\n"
      "With target names, only those targets and what they link. Tests are not built\n"
      "(see idl test). Only what changed since the last build is compiled again.\n"
      "\n"
      "Options:\n"
      USAGE_PROJECT },
    { "release", "Build the project optimized, without debug information", handle_param_release,
      "Usage: idl release [TARGET]... [OPTIONS]\n"
      "\n"
      "Like idl build, with the release profile (-O2 -DNDEBUG) into build/release/.\n"
      "\n"
      "Options:\n"
      USAGE_PROJECT },
    { "test", "Build and run the project's tests", handle_param_test,
      "Usage: idl test [TEST]... [OPTIONS]\n"
      "\n"
      "Builds (debug profile) and runs the tests: each file of tests/, or the targets\n"
      "of type test in project.yml. With names, only those tests. A test passes when\n"
      "it exits with code 0.\n"
      "\n"
      "Options:\n"
      USAGE_PROJECT },
    { "clean", "Remove the build directory", handle_param_clean,
      "Usage: idl clean [OPTIONS]\n"
      "\n"
      "Removes build/ (every profile).\n"
      "\n"
      "Options:\n"
      USAGE_PROJECT },
    { "publish", "Upload distributions to an index", handle_param_publish, nullptr },
    { "cache", "Manage idl's cache", handle_param_cache, nullptr },
    { "self", "Manage the idl executable", handle_param_self, nullptr },
    { "help", "Display documentation for a command", handle_param_help,
      "Usage: idl help [COMMAND]\n"
      "\n"
      "Without a command, lists the commands; with one, shows how to use it.\n" },
};

const int commands_count = sizeof(command_list) / sizeof(command_t);

int idl_execute_command(int argc, char *argv[]) {
    if (argc < 2) {
        char *help_argv[] = {argv[0], "help"};
        return handle_param_help(2, help_argv);
    }

    // "idl --help" alone is "idl help"; the help of a command is "idl help <command>".
    char *cmd = argv[1];
    if (argc == 2 && (strcmp(cmd, "-h") == 0 || strcmp(cmd, "--help") == 0)) {
        char *help_argv[] = { argv[0], "help" };
        return handle_param_help(2, help_argv);
    }

    const command_t *command = idl_find_command(cmd);
    if (command)
        return command->handler(argc, argv);

    printf("Unknown command: %s\n", cmd);
    return 1;
}

const command_t *idl_find_command(const char *name) {
    for (int i = 0; name && i < commands_count; i++) {
        if (strcmp(command_list[i].name, name) == 0)
            return &command_list[i];
    }
    return nullptr;
}

void idl_print_commands(void) {
    printf("Commands:\n");
    for (int i = 0; i < commands_count; i++) {
        printf("  %-8s %s\n", command_list[i].name, command_list[i].description);
    }
}
