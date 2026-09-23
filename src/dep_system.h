#ifndef IDL_DEP_SYSTEM_H
#define IDL_DEP_SYSTEM_H

#include "idl.h"

/* How a library installed on the machine was found. */
typedef enum {
    DEP_SYSTEM_BUILTIN,     // known to the toolchain: pthread, m, dl, rt
    DEP_SYSTEM_PKG_CONFIG,  // described by a .pc file
    DEP_SYSTEM_LIBRARY,     // a lib<name> file in LD_LIBRARY_PATH or a system library directory
} dep_system_kind_t;

typedef struct {
    dep_system_kind_t kind;
    char *name;             // the name it was found by: "foo" for "libfoo" found as libfoo.so
    char *version;          // pkg-config --modversion (NULL otherwise)
    char *file;             // the library file found (DEP_SYSTEM_LIBRARY)
    list_t cflags;          // compile flags: -I..., -pthread
    list_t ldflags;         // link flags: -L..., -l...
} dep_system_t;

/* Looks for a library installed on the machine, in this order: the known ones, pkg-config,
   the directories of LD_LIBRARY_PATH (DYLD_LIBRARY_PATH on macOS, PATH on Windows) and the
   system library directories. Returns false if it is not found. */
bool dep_system_find(const char *name, dep_system_t *dep);
void dep_system_clear(dep_system_t *dep);

/* A short description of where the library came from, e.g. "pkg-config, version 1.3.1". */
char *dep_system_describe(const dep_system_t *dep);

#endif // IDL_DEP_SYSTEM_H
