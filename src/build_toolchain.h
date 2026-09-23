#ifndef IDL_BUILD_TOOLCHAIN_H
#define IDL_BUILD_TOOLCHAIN_H

#include "idl.h"
#include "project_file.h"

typedef struct {
    char *cc;       // C compiler (NULL when not needed)
    char *cxx;      // C++ compiler (NULL when not needed)
    char *ar;       // static library archiver

    list_t cflags;  // compile flags from the dependencies (pkg-config --cflags)
    list_t ldflags; // link flags from the dependencies (pkg-config --libs or -l<name>)
} build_toolchain_t;

/* Picks the compilers: CC/CXX/AR from the environment, otherwise gcc/g++ and then clang/clang++. */
bool build_toolchain_init(build_toolchain_t *toolchain, bool need_c, bool need_cxx);

/* Turns project.dependencies into flags (see dep_system_find). A dependency that is not
   found is an error. */
bool build_toolchain_resolve_deps(build_toolchain_t *toolchain, list_t *dependencies);

void build_toolchain_clear(build_toolchain_t *toolchain);

#endif // IDL_BUILD_TOOLCHAIN_H
