#include "build_toolchain.h"
#include "compiler_list.h"
#include "compiler_command.h"
#include "process_runner.h"
#include "dep_system.h"

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
            log_error("No C compiler found (looked for CC, gcc and clang).");
            result = false;
        }
    }

    if (need_cxx) {
        toolchain->cxx = build_toolchain_pick(&compilers, "CXX", COMPILER_GXX, COMPILER_CLANGXX);
        if (!toolchain->cxx) {
            log_error("No C++ compiler found (looked for CXX, g++ and clang++).");
            result = false;
        }
    }

    const char *ar = getenv("AR");
    toolchain->ar = (ar && ar[0]) ? strutils_strndup(ar, strlen(ar)) : strutils_format("ar");

    compiler_list_clear(&compilers);
    return result;
}

static void build_toolchain_move(list_t *dest, list_t *src) {
    for (list_item_t *item = src->head; item; item = item->next) {
        list_add(dest, item->value);
        item->value = nullptr; // now owned by <dest>
    }
}

bool build_toolchain_resolve_deps(build_toolchain_t *toolchain, list_t *dependencies) {
    RETURN_VAL_IF_FAIL(toolchain, false);
    RETURN_VAL_IF_FAIL(dependencies, false);

    for (list_item_t *item = dependencies->head; item; item = item->next) {
        const char *name = (const char *)item->value;
        dep_system_t dep;
        if (!dep_system_find(name, &dep)) {
            log_error("Dependency '%s' not found (pkg-config, LD_LIBRARY_PATH or the system library directories).", name);
            dep_system_clear(&dep);
            return false;
        }

        log_debug("Dependency %s found", name);
        build_toolchain_move(&toolchain->cflags, &dep.cflags);
        build_toolchain_move(&toolchain->ldflags, &dep.ldflags);
        dep_system_clear(&dep);
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
