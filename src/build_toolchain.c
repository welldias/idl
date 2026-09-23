#include "build_toolchain.h"
#include "compiler_list.h"
#include "compiler_command.h"
#include "process_runner.h"

static char *build_toolchain_pick(compiler_list_t *list, const char *env, compiler_type_t first, compiler_type_t second) {
    const char *value = getenv(env);
    if (value && value[0])
        return strutils_strndup(value, strlen(value));

    compiler_info_t *compiler = compiler_list_get_by_type(list, first);
    if (!compiler)
        compiler = compiler_list_get_by_type(list, second);

    return compiler ? strutils_strndup(compiler->full_path, strlen(compiler->full_path)) : nullptr;
}

bool build_toolchain_init(build_toolchain_t *toolchain, bool need_c, bool need_cxx) {
    RETURN_VAL_IF_FAIL(toolchain, false);

    memset(toolchain, 0, sizeof(build_toolchain_t));
    list_init(&toolchain->cflags, free);
    list_init(&toolchain->ldflags, free);

    compiler_list_t compilers = {0};
    compiler_list_find(&compilers);

    bool result = true;

    if (need_c) {
        toolchain->cc = build_toolchain_pick(&compilers, "CC", COMPILER_GCC, COMPILER_CLANG);
        if (!toolchain->cc) {
            log_error("Nenhum compilador C encontrado (procurei CC, gcc e clang).");
            result = false;
        }
    }

    if (need_cxx) {
        toolchain->cxx = build_toolchain_pick(&compilers, "CXX", COMPILER_GXX, COMPILER_CLANGXX);
        if (!toolchain->cxx) {
            log_error("Nenhum compilador C++ encontrado (procurei CXX, g++ e clang++).");
            result = false;
        }
    }

    const char *ar = getenv("AR");
    toolchain->ar = (ar && ar[0]) ? strutils_strndup(ar, strlen(ar)) : strutils_format("ar");

    compiler_list_clear(&compilers);
    return result;
}

static void build_toolchain_add(list_t *list, const char *flag) {
    list_add(list, strutils_strndup(flag, strlen(flag)));
}

static void build_toolchain_add_output(list_t *list, const char *output) {
    if (output && output[0])
        strutils_str_to_list(output, strlen(output), ' ', list);
}

/* Consulta o pkg-config. Retorna false se ele não existir ou não conhecer o pacote. */
static bool build_toolchain_pkg_config(build_toolchain_t *toolchain, const char *name) {
    compiler_command_t cmds[2] = {0};
    process_job_t jobs[2] = {0};
    const char *modes[2] = { "--cflags", "--libs" };

    for (int i = 0; i < 2; i++) {
        compiler_command_init(&cmds[i], 4);
        COMPILER_COMMANDS_APPEND(&cmds[i], "pkg-config", (char *)modes[i], (char *)name);
        jobs[i].cmd = &cmds[i];
        jobs[i].capture = true;
    }

    bool found = process_runner_run(jobs, 2, 2);
    if (found) {
        build_toolchain_add_output(&toolchain->cflags, jobs[0].output);
        build_toolchain_add_output(&toolchain->ldflags, jobs[1].output);
    }

    for (int i = 0; i < 2; i++) {
        free(jobs[i].output);
        compiler_command_clear(&cmds[i]);
    }
    return found;
}

bool build_toolchain_resolve_deps(build_toolchain_t *toolchain, list_t *dependencies) {
    RETURN_VAL_IF_FAIL(toolchain, false);
    RETURN_VAL_IF_FAIL(dependencies, false);

    for (list_item_t *item = dependencies->head; item; item = item->next) {
        const char *name = (const char *)item->value;

        if (strcmp(name, "pthread") == 0 || strcmp(name, "threads") == 0) {
            build_toolchain_add(&toolchain->cflags, "-pthread");
            build_toolchain_add(&toolchain->ldflags, "-pthread");
        } else if (strcmp(name, "m") == 0 || strcmp(name, "dl") == 0 || strcmp(name, "rt") == 0) {
            char *flag = strutils_format("-l%s", name);
            list_add(&toolchain->ldflags, flag);
        } else if (!build_toolchain_pkg_config(toolchain, name)) {
            log_warn("Dependência '%s' não encontrada no pkg-config; usando -l%s.", name, name);
            list_add(&toolchain->ldflags, strutils_format("-l%s", name));
        }
    }

    return true;
}

void build_toolchain_clear(build_toolchain_t *toolchain) {
    RETURN_IF_FAIL(toolchain);

    free(toolchain->cc);
    free(toolchain->cxx);
    free(toolchain->ar);
    list_clear(&toolchain->cflags);
    list_clear(&toolchain->ldflags);
    memset(toolchain, 0, sizeof(build_toolchain_t));
}
