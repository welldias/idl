#ifndef IDL_BUILD_PLAN_H
#define IDL_BUILD_PLAN_H

#include "idl.h"
#include "build_layout.h"
#include "build_toolchain.h"

#define BUILD_OUT_DIR "build"

typedef struct {
    bool release;       // perfil release (-O2 -DNDEBUG) em vez de debug (-g -O0)
    bool with_tests;    // compila e linka também os testes de tests/
} build_options_t;

typedef enum {
    BUILD_ARTIFACT_EXE,     // src/main.* -> build/<perfil>/<nome>
    BUILD_ARTIFACT_BIN,     // src/bin/<x>.* -> build/<perfil>/<x>
    BUILD_ARTIFACT_LIB,     // projeto sem main -> build/<perfil>/lib<nome>.a
    BUILD_ARTIFACT_TEST,    // tests/<x>.* -> build/<perfil>/tests/<x>
} build_artifact_kind_t;

typedef struct {
    build_artifact_kind_t kind;
    char *name;
    char *path;
} build_artifact_t;

typedef struct build_unit_t build_unit_t;

typedef struct {
    build_options_t options;
    const char *profile;
    char *out_dir;                  // build/<perfil>

    build_layout_t layout;
    build_toolchain_t toolchain;

    build_unit_t *units;            // um por fonte compilado
    word unit_count;

    build_artifact_t *artifacts;
    word artifact_count;

    list_t strings;                 // strings alocadas usadas nos comandos
} build_plan_t;

/* Lê o projeto do diretório atual e executa o build. Retorna true em sucesso. */
bool build_plan_run(build_plan_t *plan, const build_options_t *options);
void build_plan_clear(build_plan_t *plan);

build_artifact_t *build_plan_find_artifact(build_plan_t *plan, build_artifact_kind_t kind, const char *name);

/* Entra no diretório do projeto indicado por --project (ou -p), se houver. */
bool build_enter_project_dir(const char *dir);

#endif // IDL_BUILD_PLAN_H
