#include "idl.h"
#include "cmd_args.h"
#include "build_layout.h"
#include "build_plan.h"

/* idl clean [--project <dir>] */
int handle_param_clean(int argc, char *argv[]) {
    static const char *const with_value[] = { BUILD_PROJECT_OPTIONS, nullptr };
    static const cmd_args_spec_t spec = { .with_value = with_value, .max_positionals = 0 };

    cmd_args_t args = {0};
    bool result = cmd_args_parse(&args, argc, argv, 2, &spec) && build_enter_project_dir(&args);
    cmd_args_clear(&args);

    if (!result)
        return 1;

    // Only delete build/ inside a real project, so we never remove the wrong directory.
    if (!build_layout_is_project()) {
        log_error("No project here (neither %s/ nor %s found).", BUILD_SRC_DIR, PROJECT_FILE_NAME);
        return 1;
    }

    if (!platform_dir_exists(BUILD_OUT_DIR)) {
        printf("Nothing to clean.\n");
        return 0;
    }

    if (!platform_remove_tree(BUILD_OUT_DIR))
        return 1;

    printf("Removed %s/\n", BUILD_OUT_DIR);
    return 0;
}
