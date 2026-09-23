#ifndef IDL_BUILD_PLAN_H
#define IDL_BUILD_PLAN_H

#include "idl.h"
#include "build_layout.h"
#include "build_toolchain.h"

#define BUILD_OUT_DIR "build"

typedef struct {
    bool release;       // release profile (-O2 -DNDEBUG) instead of debug (-g -O0)
    bool with_tests;    // also compile and link the tests in tests/
} build_options_t;

typedef struct {
    build_artifact_kind_t kind;
    char *name;
    char *path;
} build_artifact_t;

typedef struct build_unit_t build_unit_t;

typedef struct {
    build_options_t options;
    const char *profile;
    char *out_dir;                  // build/<profile>
    char *env_stamp;                // variables set from envs: ("NAME=value\n"...), part of every stamp

    build_layout_t layout;
    build_toolchain_t toolchain;

    build_unit_t *units;            // one per compiled source
    word unit_count;

    build_artifact_t *artifacts;
    word artifact_count;

    list_t strings;                 // allocated strings used by the commands
} build_plan_t;

/* Reads the project in the current directory and builds it. Returns true on success. */
bool build_plan_run(build_plan_t *plan, const build_options_t *options);
void build_plan_clear(build_plan_t *plan);

build_artifact_t *build_plan_find_artifact(build_plan_t *plan, build_artifact_kind_t kind, const char *name);

/* Enters the project directory given by --project (or -p), if any. */
bool build_enter_project_dir(const char *dir);

#endif // IDL_BUILD_PLAN_H
