#include "idl.h"
#include "cmd_args.h"
#include "build_plan.h"

/* idl run [<executable>] [--project <dir>] [-- program arguments]: builds (debug) and runs. */
int handle_param_run(int argc, char *argv[]) {
    int separator = argc;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0) {
            separator = i;
            break;
        }
    }

    static const char *const with_value[] = { BUILD_PROJECT_OPTIONS, nullptr };
    static const cmd_args_spec_t spec = { .with_value = with_value, .max_positionals = 1 };

    cmd_args_t args = {0};
    build_options_t options = {0};
    build_plan_t plan = {0};
    char **program_argv = nullptr;
    int exit_code = 1;

    if (!cmd_args_parse(&args, separator, argv, 2, &spec))
        goto cleanup;

    // With a name, only that executable (and what it links) is built.
    const char *bin_name = args.positionals.head ? (const char *)args.positionals.head->value : nullptr;
    options.targets = &args.positionals;

    if (!build_enter_project_dir(&args) || !build_plan_run(&plan, &options))
        goto cleanup;

    // With several executables, the one named after the project is the default.
    build_artifact_t *artifact = nullptr;
    build_artifact_t *named = nullptr;
    word executables = 0;
    for (word i = 0; i < plan.artifact_count; i++) {
        build_artifact_t *candidate = &plan.artifacts[i];
        if (candidate->kind != BUILD_ARTIFACT_EXE && candidate->kind != BUILD_ARTIFACT_BIN)
            continue;

        executables++;
        if (!artifact && (!bin_name || strcmp(candidate->name, bin_name) == 0))
            artifact = candidate;
        if (strcmp(candidate->name, plan.layout.name) == 0)
            named = candidate;
    }

    if (!bin_name && executables > 1) {
        artifact = named;
        if (!artifact) {
            log_error("There are several executables; choose one: idl run <name>.");
            goto cleanup;
        }
    }

    if (!artifact) {
        if (bin_name)
            log_error("Executable '%s' not found.", bin_name);
        else if (plan.layout.has_targets)
            log_error("The project has no executable (declare a target with type: executable in %s).", PROJECT_FILE_NAME);
        else
            log_error("The project has no executable (create src/main.c or files in src/bin/).");
        goto cleanup;
    }

    int extra = separator < argc ? argc - separator - 1 : 0;
    program_argv = (char **)calloc(extra + 2, sizeof(char *));
    if (!program_argv) {
        LOG_FATAL_NOT_ENOUGH_MEMORY();
        goto cleanup;
    }

    program_argv[0] = artifact->path;
    for (int i = 0; i < extra; i++)
        program_argv[i + 1] = argv[separator + 1 + i];

    printf("Running %s\n", artifact->path);
    fflush(stdout);
    exit_code = platform_exec(program_argv);
    if (exit_code < 0)
        exit_code = 1;

cleanup:
    free(program_argv);
    build_plan_clear(&plan);
    cmd_args_clear(&args);
    return exit_code;
}
