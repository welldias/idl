#include "dep_system.h"
#include "compiler_command.h"
#include "process_runner.h"

#include <uv.h>

static void dep_system_add(list_t *list, const char *flag) {
    list_add(list, strutils_format("%s", flag));
}

/*****************************************************************************/
/* Known libraries                                                           */
/*****************************************************************************/

static bool dep_system_builtin(const char *name, dep_system_t *dep) {
    if (strcmp(name, "pthread") == 0 || strcmp(name, "threads") == 0) {
        dep_system_add(&dep->cflags, "-pthread");
        dep_system_add(&dep->ldflags, "-pthread");
    } else if (strcmp(name, "m") == 0 || strcmp(name, "dl") == 0 || strcmp(name, "rt") == 0) {
        list_add(&dep->ldflags, strutils_format("-l%s", name));
    } else {
        return false;
    }
    dep->kind = DEP_SYSTEM_BUILTIN;
    return true;
}

/*****************************************************************************/
/* pkg-config                                                                */
/*****************************************************************************/

static bool dep_system_pkg_config(const char *name, dep_system_t *dep) {
    const char *modes[] = { "--cflags", "--libs", "--modversion" };
    compiler_command_t cmds[3] = {0};
    process_job_t jobs[3] = {0};

    for (int i = 0; i < 3; i++) {
        compiler_command_init(&cmds[i], 4);
        COMPILER_COMMANDS_APPEND(&cmds[i], "pkg-config", (char *)modes[i], (char *)name);
        jobs[i].cmd = &cmds[i];
        jobs[i].capture = true;
    }

    // A missing pkg-config or an unknown package both mean "not found here".
    bool found = process_runner_run(jobs, 3, 3);
    if (found) {
        if (jobs[0].output)
            strutils_str_to_list(jobs[0].output, strlen(jobs[0].output), ' ', &dep->cflags);
        if (jobs[1].output)
            strutils_str_to_list(jobs[1].output, strlen(jobs[1].output), ' ', &dep->ldflags);
        if (jobs[2].output) {
            dep->version = strutils_format("%s", jobs[2].output);
            strutils_trim(dep->version);
        }
        dep->kind = DEP_SYSTEM_PKG_CONFIG;
    }

    for (int i = 0; i < 3; i++) {
        free(jobs[i].output);
        compiler_command_clear(&cmds[i]);
    }
    return found;
}

/*****************************************************************************/
/* Library directories                                                       */
/*****************************************************************************/

/* The files the linker accepts for -l<name>; a versioned libfoo.so.1 alone is not one of them. */
static const char *const dep_system_patterns[] = {
#if defined(_WIN32)
    "lib%s.dll.a", "lib%s.a", "%s.lib", "lib%s.dll",
#elif defined(__APPLE__)
    "lib%s.dylib", "lib%s.tbd", "lib%s.a",
#else
    "lib%s.so", "lib%s.a",
#endif
};

static const char *const dep_system_standard_dirs[] = {
#if defined(__APPLE__)
    "/opt/homebrew/lib", "/usr/local/lib", "/usr/lib",
#elif !defined(_WIN32)
    "/usr/local/lib64", "/usr/local/lib", "/usr/lib64", "/usr/lib", "/lib64", "/lib",
#endif
};

static const char *const dep_system_path_vars[] = {
#if defined(_WIN32)
    "PATH",
#elif defined(__APPLE__)
    "DYLD_LIBRARY_PATH", "LD_LIBRARY_PATH",
#else
    "LD_LIBRARY_PATH",
#endif
};

typedef struct {
    list_t dirs;        // char *, in search order
    word user_count;    // the first ones come from the environment; the others are standard
} dep_system_dirs_t;

static bool dep_system_same_dir(const void *value, const void *item_value) {
    return strcmp((const char *)value, (const char *)item_value) == 0;
}

static void dep_system_add_dir(dep_system_dirs_t *dirs, const char *dir) {
    if (dir[0] && !list_contains(&dirs->dirs, dir, dep_system_same_dir))
        list_add(&dirs->dirs, strutils_format("%s", dir));
}

/* Debian and Ubuntu keep the libraries in /usr/lib/<triplet> (e.g. /usr/lib/x86_64-linux-gnu). */
static void dep_system_add_multiarch(dep_system_dirs_t *dirs) {
#if !defined(_WIN32) && !defined(__APPLE__)
    uv_fs_t req;
    if (uv_fs_scandir(nullptr, &req, "/usr/lib", 0, nullptr) >= 0) {
        uv_dirent_t entry;
        while (uv_fs_scandir_next(&req, &entry) != UV_EOF) {
            if (entry.type == UV_DIRENT_DIR && strstr(entry.name, "-linux-gnu")) {
                char *dir = strutils_format("/usr/lib/%s", entry.name);
                dep_system_add_dir(dirs, dir);
                free(dir);
            }
        }
    }
    uv_fs_req_cleanup(&req);
#else
    (void)dirs;
#endif
}

static void dep_system_collect_dirs(dep_system_dirs_t *dirs) {
    list_init(&dirs->dirs, free);

    for (word v = 0; v < SIZE_OF_ARRAY(dep_system_path_vars); v++) {
        const char *value = getenv(dep_system_path_vars[v]);
        if (!value || !value[0])
            continue;

        list_t entries = {0};
        list_init(&entries, free);
        strutils_str_to_list(value, strlen(value), ENV_PATH_SEPARATOR, &entries);
        for (list_item_t *item = entries.head; item; item = item->next)
            dep_system_add_dir(dirs, (const char *)item->value);
        list_clear(&entries);
    }
    dirs->user_count = dirs->dirs.count;

    dep_system_add_multiarch(dirs);
    for (word i = 0; i < SIZE_OF_ARRAY(dep_system_standard_dirs); i++)
        dep_system_add_dir(dirs, dep_system_standard_dirs[i]);
}

static bool dep_system_in_dir(const char *dir, const char *lib, bool standard, dep_system_t *dep) {
    for (word p = 0; p < SIZE_OF_ARRAY(dep_system_patterns); p++) {
        char *file_name = strutils_format(dep_system_patterns[p], lib);
        char *path = strutils_format("%s/%s", dir, file_name);
        free(file_name);
        if (!path || !platform_file_exists(path)) {
            free(path);
            continue;
        }

        dep->kind = DEP_SYSTEM_LIBRARY;
        dep->file = path;
        if (!standard) {
            list_add(&dep->ldflags, strutils_format("-L%s", dir));

            // /opt/foo/lib/libfoo.so usually comes with /opt/foo/include.
            char *include = strutils_format("%s/../include", dir);
            if (include && platform_dir_exists(include))
                list_add(&dep->cflags, strutils_format("-I%s", include));
            free(include);
        }
        list_add(&dep->ldflags, strutils_format("-l%s", lib));
        return true;
    }
    return false;
}

static bool dep_system_library(const char *name, dep_system_t *dep) {
    dep_system_dirs_t dirs = {0};
    dep_system_collect_dirs(&dirs);

    // "idl add libfoo" means the library foo.
    const char *names[2] = { name, strncmp(name, "lib", 3) == 0 && name[3] ? name + 3 : nullptr };

    bool found = false;
    for (word n = 0; n < 2 && !found && names[n]; n++) {
        word index = 0;
        for (list_item_t *item = dirs.dirs.head; item && !found; item = item->next, index++)
            found = dep_system_in_dir((const char *)item->value, names[n], index >= dirs.user_count, dep);
        if (found) {
            free(dep->name);
            dep->name = strutils_format("%s", names[n]);
        }
    }

    list_clear(&dirs.dirs);
    return found;
}

/*****************************************************************************/
/* API                                                                       */
/*****************************************************************************/

bool dep_system_find(const char *name, dep_system_t *dep) {
    RETURN_VAL_IF_FAIL(name, false);
    RETURN_VAL_IF_FAIL(dep, false);

    memset(dep, 0, sizeof(dep_system_t));
    list_init(&dep->cflags, free);
    list_init(&dep->ldflags, free);

    if (!name[0])
        return false;
    dep->name = strutils_format("%s", name);
    return dep_system_builtin(name, dep) || dep_system_pkg_config(name, dep) || dep_system_library(name, dep);
}

void dep_system_clear(dep_system_t *dep) {
    RETURN_IF_FAIL(dep);

    free(dep->name);
    free(dep->version);
    free(dep->file);
    list_clear(&dep->cflags);
    list_clear(&dep->ldflags);
    memset(dep, 0, sizeof(dep_system_t));
}

char *dep_system_describe(const dep_system_t *dep) {
    RETURN_VAL_IF_FAIL(dep, nullptr);

    switch (dep->kind) {
        case DEP_SYSTEM_BUILTIN:
            return strutils_format("system library");
        case DEP_SYSTEM_PKG_CONFIG:
            return dep->version && dep->version[0] ? strutils_format("pkg-config, version %s", dep->version)
                                                   : strutils_format("pkg-config");
        default:
            return strutils_format("%s", dep->file ? dep->file : "library file");
    }
}
