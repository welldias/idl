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
    char *path;         // relative to the project root, always with '/' (e.g. "src/util.c")
    char *stem;         // file name without directory and extension (e.g. "util")
    build_lang_t lang;
} build_source_t;

typedef struct {
    build_source_t *items;
    word count;
    word capacity;
} build_sources_t;

/* What the directory convention (the "labels") tells about the project. */
typedef struct {
    char *name;                 // project.name or the directory name
    bool has_config;            // project.yml exists
    project_config_t config;    // empty when there is no project.yml
    bool has_include_dir;       // include/ exists

    build_sources_t main;       // src/main.{c,cpp,...}: 0 or 1 item
    build_sources_t lib;        // other sources in src/ (no main, no bin/)
    build_sources_t bins;       // src/bin/<name>.{c,cpp,...}: one executable each
    build_sources_t tests;      // tests/<name>.{c,cpp,...}: one test each
} build_layout_t;

/* Reads the project structure in the current directory. */
bool build_layout_load(build_layout_t *layout, bool with_tests);
void build_layout_clear(build_layout_t *layout);

bool build_layout_uses_lang(build_layout_t *layout, build_lang_t lang, bool with_tests);

/* Tells whether the path is a known C/C++ source and its language. */
bool build_source_lang(const char *path, build_lang_t *lang);

/* Operating systems a platform-specific source can target (bit mask). */
enum {
    BUILD_OS_WINDOWS = 1 << 0,
    BUILD_OS_LINUX = 1 << 1,
    BUILD_OS_MACOS = 1 << 2,
    BUILD_OS_UNIX = 1 << 3,  // any Unix-like system, Linux and macOS included
};

/* The systems idl is running on (e.g. BUILD_OS_LINUX | BUILD_OS_UNIX). */
unsigned build_os_host(void);

/* Tells whether the source is built on the given systems: a file name ending in
   _win, _linux, _macos or _unix (e.g. "src/io_win.c") only builds on that system. */
bool build_source_for_os(const char *path, unsigned os);

/* Tells whether the current directory looks like an idl project (has src/ or project.yml). */
bool build_layout_is_project(void);

#endif // IDL_BUILD_LAYOUT_H
