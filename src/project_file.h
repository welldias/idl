#ifndef IDL_PROJECT_FILE_H
#define IDL_PROJECT_FILE_H

#include "idl.h"

#define PROJECT_FILE_NAME "project.yml"
#define PROJECT_NAME_LEN 100
#define PROJECT_VERSION_LEN 20
#define PROJECT_DESCRIPTION_LEN 1000
#define PROJECT_REQUIRES_C_LEN 20

/* Optional "build:" section of project.yml. All lists hold strings. */
typedef struct {
    list_t defines;       // FOO=1 -> -DFOO=1
    list_t include_dirs;  // -I<dir>
    list_t cflags;        // extra flags for C
    list_t cxxflags;      // extra flags for C++
    list_t ldflags;       // extra linker flags
    list_t libs;          // -l<lib>, without going through pkg-config
} project_build_config_t;

typedef enum {
    PROJECT_TARGET_EXECUTABLE,
    PROJECT_TARGET_STATIC_LIBRARY,
    PROJECT_TARGET_SHARED_LIBRARY,
    PROJECT_TARGET_LIBRARY,          // static and shared, from the same objects
} project_target_type_t;

/* An item of the "targets:" section: what to compile and how. All lists hold strings. */
typedef struct {
    char *name;                   // key of the item in "targets:"
    project_target_type_t type;
    list_t sources;               // files and globs (*, ?, **)
    list_t exclude;               // globs removed from sources
    list_t include_dirs;          // -I<dir> for this target only
    list_t public_include_dirs;   // -I<dir> for this target and the targets that link it
    list_t defines;
    list_t cflags;
    list_t cxxflags;
    list_t ldflags;
    list_t libs;                  // -l<lib>
    list_t link;                  // names of other targets
} project_target_config_t;

/* An item of the "envs:" section. Only uppercase names become environment variables. */
typedef struct {
    char *name;
    char *value;
} project_env_t;

typedef struct {
    char *name;
    char *version;
    char *description;
    char *requires_c;
    char *requires_cpp;
    list_t dependencies;  // system libraries (pkg-config or -l)
    project_build_config_t build;
    list_t targets;       // project_target_config_t *; empty: the directory convention is used
    list_t envs;          // project_env_t *, in the order of the file
} project_config_t;

const char *project_target_type_name(project_target_type_t type);

void project_file_config_init(project_config_t *config);
void project_file_config_clean(project_config_t *config);

bool project_file_save(project_config_t *config);
bool project_file_read(project_config_t *config);

bool project_file_name_set(project_config_t *config, const char *value);
bool project_file_version_set(project_config_t *config, const char *value);
bool project_file_description_set(project_config_t *config, const char *value);
bool project_file_requires_c_set(project_config_t *config, const char *value);
bool project_file_requires_cpp_set(project_config_t *config, const char *value);

bool project_file_dependency_add(project_config_t *config, const char *value);
bool project_file_dependency_find(project_config_t *config, const char *value);
bool project_file_dependency_remove(project_config_t *config, const char *value);

bool project_file_exist();

#endif // IDL_PROJECT_FILE_H