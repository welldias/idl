#ifndef IDL_BUILD_TARGET_H
#define IDL_BUILD_TARGET_H

#include "idl.h"
#include "build_source.h"
#include "project_file.h"

typedef enum {
    BUILD_ARTIFACT_EXE,     // main executable (src/main.* or an executable target)
    BUILD_ARTIFACT_BIN,     // src/bin/<x>.* -> build/<profile>/<x>
    BUILD_ARTIFACT_LIB,     // static library -> build/<profile>/lib<name>.a
    BUILD_ARTIFACT_TEST,    // tests/<x>.* -> build/<profile>/tests/<x>
    BUILD_ARTIFACT_SHARED,  // shared library -> build/<profile>/lib<name>.so (.dylib, .dll)
} build_artifact_kind_t;

typedef enum {
    BUILD_TARGET_OBJECTS,         // no artifact: its objects go into the targets that link it
    BUILD_TARGET_EXECUTABLE,
    BUILD_TARGET_STATIC_LIBRARY,
    BUILD_TARGET_SHARED_LIBRARY,
    BUILD_TARGET_LIBRARY,         // static and shared, from the same objects
} build_target_type_t;

/* Something to compile and link: an executable, a library or a group of objects.
   Both the directory convention and the "targets:" section of project.yml produce these. */
typedef struct {
    char *name;
    build_target_type_t type;
    build_artifact_kind_t exe_kind;  // executables: EXE, BIN or TEST
    bool link_objects;               // libraries: the targets that link it get its objects, not the archive
    char *obj_dir;                   // subdirectory of obj/ for its objects ("" or "exe/<name>/", "lib/<name>/")
    build_sources_t sources;

    list_t include_dirs;             // -I for this target
    list_t public_include_dirs;      // -I for this target and the targets that link it
    list_t defines;
    list_t cflags;
    list_t cxxflags;
    list_t ldflags;
    list_t libs;
    list_t link;                     // names of the targets it links

    // Filled by build_targets_resolve.
    word *deps;                      // indexes of the linked targets
    word dep_count;
    word level;                      // 0: links nothing; otherwise 1 + the level of its deepest dependency
    bool pic;                        // its objects go into a shared library

    word first_unit;                 // filled by the build plan
} build_target_t;

typedef struct {
    build_target_t *items;
    word count;
    word capacity;
} build_targets_t;

/* Adds an empty target. The returned pointer is valid until the next add. */
build_target_t *build_targets_add(build_targets_t *targets, const char *name, build_target_type_t type);
void build_targets_clear(build_targets_t *targets);

/* Creates the targets of the "targets:" section, expanding their sources. Targets of type
   test are only created <with_tests>: one executable per source, or one with all (single). */
bool build_targets_from_config(build_targets_t *targets, project_config_t *config, bool with_tests);

/* Resolves the links by name, rejects invalid links and cycles, and computes level and pic. */
bool build_targets_resolve(build_targets_t *targets);

bool build_target_is_library(const build_target_t *target);

/* Targets reached through the links of <index>, dependents before their dependencies
   (the order the linker needs). With <for_link>, the search does not look inside shared
   libraries, except for other shared libraries they need. Returns the count written to <out>,
   which must hold targets->count items. */
word build_target_closure(build_targets_t *targets, word index, bool for_link, word *out);

#endif // IDL_BUILD_TARGET_H
