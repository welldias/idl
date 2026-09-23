#ifndef IDL_BUILD_LAYOUT_H
#define IDL_BUILD_LAYOUT_H

#include "idl.h"
#include "project_file.h"
#include "build_source.h"
#include "build_target.h"

#define BUILD_SRC_DIR "src"
#define BUILD_BIN_DIR "src/bin"
#define BUILD_INCLUDE_DIR "include"
#define BUILD_TESTS_DIR "tests"

/* What the project is made of: the targets declared in project.yml or, without them,
   those the directory convention (the "labels") implies. */
typedef struct {
    char *name;                 // project.name or the directory name
    bool has_config;            // project.yml exists
    project_config_t config;    // empty when there is no project.yml
    bool has_targets;           // project.yml declares targets: the convention is not used

    // Directory convention only.
    bool has_include_dir;       // include/ exists
    build_sources_t main;       // src/main.{c,cpp,...}: 0 or 1 item
    build_sources_t lib;        // other sources in src/ (no main, no bin/)
    build_sources_t bins;       // src/bin/<name>.{c,cpp,...}: one executable each
    build_sources_t tests;      // tests/<name>.{c,cpp,...}: one test each

    build_targets_t targets;    // what gets built, links already resolved
} build_layout_t;

/* Reads the project in the current directory. The tests are only included with <with_tests>. */
bool build_layout_load(build_layout_t *layout, bool with_tests);
void build_layout_clear(build_layout_t *layout);

bool build_layout_uses_lang(build_layout_t *layout, build_lang_t lang);

/* Tells whether the current directory looks like an idl project (has src/ or project.yml). */
bool build_layout_is_project(void);

#endif // IDL_BUILD_LAYOUT_H
