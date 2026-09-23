#include "cmd_handlers.h"
#include "cmd_command.h"

const command_t command_list[] = {    
    { "run", "Build and run the project's executable", handle_param_run }, 
    { "init", "Create a new project", handle_param_init }, 
    { "add", "Add dependencies to the project", handle_param_add }, 
    { "remove", "Remove dependencies from the project", handle_param_remove }, 
    { "version", "Read or update the project's version", handle_param_version }, 
    { "sync", "Update the project's environment", handle_param_sync }, 
    { "lock", "Update the project's lockfile", handle_param_lock }, 
    { "export", "Export the project's lockfile to an alternate format", handle_param_export }, 
    { "tree", "Display the project's dependency tree", handle_param_tree }, 
    { "tool", "Run and install commands provided by C packages", handle_param_tool }, 
    { "c", "Manage C versions and installations", handle_param_c }, 
    { "pip", "Manage C packages with a pip-compatible interface", handle_param_pip }, 
    { "venv", "Create a virtual environment", handle_param_venv }, 
    { "build", "Build the project", handle_param_build }, 
    { "test", "Build and run the project's tests", handle_param_test }, 
    { "clean", "Remove the build directory", handle_param_clean }, 
    { "publish", "Upload distributions to an index", handle_param_publish }, 
    { "cache", "Manage bx's cache", handle_param_cache }, 
    { "self", "Manage the bx executable", handle_param_self }, 
    { "help", "Display documentation for a command", handle_param_help }, 
};

const int commands_count = sizeof(command_list) / sizeof(command_t);

int idl_execute_command(int argc, char *argv[]) {
    if (argc < 2) {
        char *help_argv[] = {argv[0], "help"};
        return handle_param_help(2, help_argv);
    }

    char *cmd = argv[1];
    
    for (int i = 0; i < commands_count; i++) {
        if (strcmp(command_list[i].name, cmd) == 0) {
            return command_list[i].handler(argc, argv);
        }
    }

    printf("Unknown command: %s\n", cmd);
    return 1;
}

void idl_print_commands(void) {
    printf("Commands:\n");
    for (int i = 0; i < commands_count; i++) {
        printf("  %-8s %s\n", command_list[i].name, command_list[i].description);
    }
}
