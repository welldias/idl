#ifndef IDL_BUILD_SOURCE_H
#define IDL_BUILD_SOURCE_H

#include "idl.h"

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

bool build_sources_add(build_sources_t *sources, const char *path, build_lang_t lang);
bool build_sources_contains(build_sources_t *sources, const char *path);
void build_sources_sort(build_sources_t *sources);
void build_sources_clear(build_sources_t *sources);

#endif // IDL_BUILD_SOURCE_H
