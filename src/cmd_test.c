#include "idl.h"
#include "cmd_args.h"
#include "build_plan.h"

/* idl test [--release] [--project <dir>]: builds and runs each file in tests/. */
int handle_param_test(int argc, char *argv[]) {
    cmd_args_t args = {0};
    cmd_args_parse(&args, argc, argv, 2);

    build_options_t options = {
        .release = cmd_args_has_flag(&args, "release"),
        .with_tests = true,
    };

    build_plan_t plan = {0};
    int exit_code = 1;

    if (!build_enter_project_dir(cmd_args_get_value(&args, "project")) || !build_plan_run(&plan, &options))
        goto cleanup;

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
