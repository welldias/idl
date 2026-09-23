#include "idl.h"
#include "cmd_args.h"
#include "build_plan.h"

/* build and release: the same command in a different profile. */
static int cmd_build_profile(int argc, char *argv[], bool release) {
    static const char *const with_value[] = { BUILD_PROJECT_OPTIONS, nullptr };
    static const cmd_args_spec_t spec = { .with_value = with_value, .max_positionals = -1 };

    cmd_args_t args = {0};
    build_options_t options = { .release = release };
    build_plan_t plan = {0};

    bool result = cmd_args_parse(&args, argc, argv, 2, &spec);
    if (result) {
        options.targets = &args.positionals;
        result = build_enter_project_dir(&args) && build_plan_run(&plan, &options);
    }

    build_plan_clear(&plan);
    cmd_args_clear(&args);
    return result ? 0 : 1;
}

/* idl build [<target>...] [--project <dir>]: the debug profile (-g -O0), in build/debug/. */
int handle_param_build(int argc, char *argv[]) {
    return cmd_build_profile(argc, argv, false);
}

/* idl release [<target>...] [--project <dir>]: the release profile (-O2 -DNDEBUG), in build/release/. */
int handle_param_release(int argc, char *argv[]) {
    return cmd_build_profile(argc, argv, true);
}
