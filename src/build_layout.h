#ifndef IDL_BUILD_LAYOUT_H
#define IDL_BUILD_LAYOUT_H

#include "idl.h"
#include "project_file.h"

#define BUILD_SRC_DIR "src"
#define BUILD_BIN_DIR "src/bin"
#define BUILD_INCLUDE_DIR "include"
#define BUILD_TESTS_DIR "tests"

typedef enum {
    BUILD_LANG_C,
    BUILD_LANG_CXX,
} build_lang_t;

typedef struct {
    char *path;         // relativo à raiz do projeto, sempre com '/' (ex.: "src/util.c")
    char *stem;         // nome do arquivo sem diretório e sem extensão (ex.: "util")
    build_lang_t lang;
} build_source_t;

typedef struct {
    build_source_t *items;
    word count;
    word capacity;
} build_sources_t;

/* O que a convenção de diretórios (as "etiquetas") revela sobre o projeto. */
typedef struct {
    char *name;                 // project.name ou o nome do diretório
    bool has_config;            // existe project.yml
    project_config_t config;    // vazio se não houver project.yml
    bool has_include_dir;       // existe include/

    build_sources_t main;       // src/main.{c,cpp,...}: 0 ou 1 item
    build_sources_t lib;        // demais fontes de src/ (sem main e sem bin/)
    build_sources_t bins;       // src/bin/<nome>.{c,cpp,...}: um executável cada
    build_sources_t tests;      // tests/<nome>.{c,cpp,...}: um teste cada
} build_layout_t;

/* Lê a estrutura do projeto no diretório atual. */
bool build_layout_load(build_layout_t *layout, bool with_tests);
void build_layout_clear(build_layout_t *layout);

bool build_layout_uses_lang(build_layout_t *layout, build_lang_t lang, bool with_tests);

/* Indica se o caminho é um fonte C/C++ reconhecido e qual a linguagem. */
bool build_source_lang(const char *path, build_lang_t *lang);

/* Diz se o diretório atual parece um projeto idl (tem src/ ou project.yml). */
bool build_layout_is_project(void);

#endif // IDL_BUILD_LAYOUT_H
