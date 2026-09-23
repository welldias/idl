#include "idl.h"
#include "cmd_args.h"
#include "build_plan.h"

/* idl run [--release] [--bin <nome>] [--project <dir>] [-- argumentos do programa] */
int handle_param_run(int argc, char *argv[]) {
    int separator = argc;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0) {
            separator = i;
            break;
        }
    }

    cmd_args_t args = {0};
    cmd_args_parse(&args, separator, argv, 2);

    build_options_t options = {
        .release = cmd_args_has_flag(&args, "release"),
    };
    const char *bin_name = cmd_args_get_value(&args, "bin");

    int exit_code = 1;
    build_plan_t plan = {0};
    char **program_argv = nullptr;

    if (!build_enter_project_dir(cmd_args_get_value(&args, "project")) || !build_plan_run(&plan, &options))
        goto cleanup;

    build_artifact_t *artifact = nullptr;
    if (bin_name) {
        artifact = build_plan_find_artifact(&plan, BUILD_ARTIFACT_BIN, bin_name);
        if (!artifact)
            artifact = build_plan_find_artifact(&plan, BUILD_ARTIFACT_EXE, bin_name);
    } else {
        artifact = build_plan_find_artifact(&plan, BUILD_ARTIFACT_EXE, nullptr);
        if (!artifact) {
            word bins = 0;
            for (word i = 0; i < plan.artifact_count; i++) {
                if (plan.artifacts[i].kind == BUILD_ARTIFACT_BIN) {
                    artifact = &plan.artifacts[i];
                    bins++;
                }
            }
            if (bins > 1) {
                log_error("Há vários executáveis em src/bin/; escolha um com --bin <nome>.");
                goto cleanup;
            }
        }
    }

    if (!artifact) {
        if (bin_name)
            log_error("Executável '%s' não encontrado.", bin_name);
        else
            log_error("O projeto não gera executável (crie src/main.c ou arquivos em src/bin/).");
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

    printf("Executando %s\n", artifact->path);
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
