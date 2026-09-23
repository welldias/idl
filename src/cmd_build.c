#include "idl.h"
#include "cmd_args.h"
#include "build_plan.h"

int handle_param_build(int argc, char *argv[]) {
    cmd_args_t args = {0};
    cmd_args_parse(&args, argc, argv, 2);

    const char *project_dir = cmd_args_get_value(&args, "project");
    if (!project_dir)
        project_dir = cmd_args_get_value(&args, "p");

    build_options_t options = {
        .release = cmd_args_has_flag(&args, "release"),
    };

    build_plan_t plan = {0};
    bool result = build_enter_project_dir(project_dir) && build_plan_run(&plan, &options);

    build_plan_clear(&plan);
    cmd_args_clear(&args);
    return result ? 0 : 1;
}
