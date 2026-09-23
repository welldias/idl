#include "idl.h"
#include "cmd_args.h"
#include "build_layout.h"
#include "build_plan.h"

int handle_param_clean(int argc, char *argv[]) {
    cmd_args_t args = {0};
    cmd_args_parse(&args, argc, argv, 2);

    const char *project_dir = cmd_args_get_value(&args, "project");
    bool result = build_enter_project_dir(project_dir);
    cmd_args_clear(&args);

    if (!result)
        return 1;

    // Só apaga build/ se aqui for mesmo um projeto, para não remover o diretório errado.
    if (!build_layout_is_project()) {
        log_error("Nenhum projeto aqui (não há %s/ nem %s).", BUILD_SRC_DIR, PROJECT_FILE_NAME);
        return 1;
    }

    if (!platform_dir_exists(BUILD_OUT_DIR)) {
        printf("Nada a limpar.\n");
        return 0;
    }

    if (!platform_remove_tree(BUILD_OUT_DIR))
        return 1;

    printf("Removido %s/\n", BUILD_OUT_DIR);
    return 0;
}
