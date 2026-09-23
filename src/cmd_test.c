#include "idl.h"
#include "cmd_args.h"
#include "build_plan.h"

/* idl test [<test>...] [--project <dir>]: builds (debug) and runs the tests: each file in tests/,
   or the targets of type test in project.yml. With names, only those tests. */
int handle_param_test(int argc, char *argv[]) {
    static const char *const with_value[] = { BUILD_PROJECT_OPTIONS, nullptr };
    static const cmd_args_spec_t spec = { .with_value = with_value, .max_positionals = -1 };

    cmd_args_t args = {0};
    build_options_t options = { .with_tests = true };
    build_plan_t plan = {0};
    int exit_code = 1;

    if (!cmd_args_parse(&args, argc, argv, 2, &spec))
        goto cleanup;
    options.targets = &args.positionals;

    if (!build_enter_project_dir(&args) || !build_plan_run(&plan, &options))
        goto cleanup;

#if defined(_WIN32)
    // The tests are in build/<profile>/tests: Windows finds the project's DLLs through PATH.
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd))) {
        const char *path = getenv("PATH");
        char *value = strutils_format("%s\\%s;%s", cwd, plan.out_dir, path ? path : "");
        uv_os_setenv("PATH", value);
        free(value);
    }
#endif

    word passed = 0;
    word failed = 0;
    for (word i = 0; i < plan.artifact_count; i++) {
        build_artifact_t *test = &plan.artifacts[i];
        if (test->kind != BUILD_ARTIFACT_TEST)
            continue;

        printf("\nTest %s\n", test->name);
        fflush(stdout);

        char *test_argv[] = { test->path, nullptr };
        int code = platform_exec(test_argv);
        if (code == 0) {
            printf("Test %s: ok\n", test->name);
            passed++;
        } else {
            printf("Test %s: FAILED (exit code %d)\n", test->name, code);
            failed++;
        }
    }

    if (passed + failed == 0) {
        if (plan.layout.has_targets)
            printf("No tests declared (targets of type test in %s).\n", PROJECT_FILE_NAME);
        else
            printf("No tests found in %s/.\n", BUILD_TESTS_DIR);
        exit_code = 0;
        goto cleanup;
    }

    printf("\nResult: %zu passed, %zu failed.\n", passed, failed);
    exit_code = failed ? 1 : 0;

cleanup:
    build_plan_clear(&plan);
    cmd_args_clear(&args);
    return exit_code;
}
