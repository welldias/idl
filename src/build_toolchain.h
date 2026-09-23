#ifndef IDL_BUILD_TOOLCHAIN_H
#define IDL_BUILD_TOOLCHAIN_H

#include "idl.h"
#include "project_file.h"

typedef struct {
    char *cc;       // compilador C (NULL se não for necessário)
    char *cxx;      // compilador C++ (NULL se não for necessário)
    char *ar;       // arquivador de bibliotecas estáticas

    list_t cflags;  // flags de compilação vindas das dependências (pkg-config --cflags)
    list_t ldflags; // flags de link vindas das dependências (pkg-config --libs ou -l<nome>)
} build_toolchain_t;

/* Escolhe os compiladores: CC/CXX/AR do ambiente ou, senão, gcc/g++ e depois clang/clang++. */
bool build_toolchain_init(build_toolchain_t *toolchain, bool need_c, bool need_cxx);

/* Converte project.dependencies em flags (pkg-config, com -l<nome> como alternativa). */
bool build_toolchain_resolve_deps(build_toolchain_t *toolchain, list_t *dependencies);

void build_toolchain_clear(build_toolchain_t *toolchain);

#endif // IDL_BUILD_TOOLCHAIN_H
