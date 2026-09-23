#include "project_file.h"
#include "cmd_args.h"
#include "build_env.h"
#include "dep_system.h"

/* idl add <dependency>...: looks for each library on the machine and adds it to
   project.dependencies. Nothing is saved if one of them is not found. */
int handle_param_add(int argc, char *argv[]) {
    static const cmd_args_spec_t spec = { .max_positionals = -1 };
    cmd_args_t args = {0};
    project_config_t config = {0};
    project_file_config_init(&config);
    char *applied = nullptr;
    bool result = false;

    if (!cmd_args_parse(&args, argc, argv, 2, &spec))
        goto cleanup;

    if (args.positionals.count == 0) {
        fprintf(stderr, "Usage: idl add <dependency>...\n");
        goto cleanup;
    }

    if (project_file_exist() == false) {
        fprintf(stderr, "Project file not found\n");
        goto cleanup;
    }

    // The envs: of the project (PKG_CONFIG_PATH, LD_LIBRARY_PATH...) also count for the search.
    if (!project_file_read(&config) || !build_env_apply(&config.envs, &applied))
        goto cleanup;

    bool all_found = true;
    word added = 0;
    for (list_item_t *item = args.positionals.head; item; item = item->next) {
        const char *name = (const char *)item->value;
        if (project_file_dependency_find(&config, name)) {
            printf("'%s' is already a dependency.\n", name);
            continue;
        }

        dep_system_t dep;
        if (!dep_system_find(name, &dep)) {
            log_error("Library '%s' not found (pkg-config, LD_LIBRARY_PATH or the system library directories).", name);
            all_found = false;
        } else if (strcmp(dep.name, name) != 0 && project_file_dependency_find(&config, dep.name)) {
            printf("'%s' is already a dependency (as '%s').\n", name, dep.name);
        } else {
            // "libfoo" found as libfoo.so is saved as "foo", the name the linker knows.
            char *where = dep_system_describe(&dep);
            printf("Added %s (%s)\n", dep.name, where);
            free(where);
            project_file_dependency_add(&config, dep.name);
            added++;
        }
        dep_system_clear(&dep);
    }

    if (!all_found) {
        if (added)
            printf("Nothing was saved: add the libraries that were found without the others.\n");
        goto cleanup;
    }

    result = added == 0 || project_file_save(&config);

cleanup:
    free(applied);
    project_file_config_clean(&config);
    cmd_args_clear(&args);
    return result ? 0 : 1;
}
