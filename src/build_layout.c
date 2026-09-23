#include "build_layout.h"

typedef struct {
    build_layout_t *layout;
    const char *root;   // directory being scanned ("src" or "tests")
    bool ok;
} build_layout_scan_t;

static void build_layout_on_file(const char *full_path, void *arg) {
    build_layout_scan_t *scan = (build_layout_scan_t *)arg;
    build_layout_t *layout = scan->layout;

    char path[1024];
    snprintf(path, sizeof(path), "%s", full_path);
    for (char *p = path; *p; p++) {
        if (*p == '\\')
            *p = '/';
    }

    build_lang_t lang;
    if (!build_source_lang(path, &lang))
        return;

    if (!build_source_for_os(path, build_os_host())) {
        log_debug("%s skipped: built only on another system.", path);
        return;
    }

    const char *rest = path + strlen(scan->root) + 1; // path inside src/ or tests/
    build_sources_t *target = nullptr;

    if (strcmp(scan->root, BUILD_TESTS_DIR) == 0) {
        if (strchr(rest, '/')) {
            log_warn("%s ignored: only files directly in tests/ become tests.", path);
            return;
        }
        target = &layout->tests;
    } else if (strncmp(rest, "bin/", 4) == 0) {
        if (strchr(rest + 4, '/')) {
            log_warn("%s ignored: only files directly in src/bin/ become executables.", path);
            return;
        }
        target = &layout->bins;
    } else if (!strchr(rest, '/') && strncmp(rest, "main.", 5) == 0) {
        target = &layout->main;
    } else {
        target = &layout->lib;
    }

    if (!build_sources_add(target, path, lang))
        scan->ok = false;
}

static bool build_layout_scan(build_layout_t *layout, const char *root) {
    build_layout_scan_t scan = { .layout = layout, .root = root, .ok = true };
    return platform_scan_directory(root, nullptr, build_layout_on_file, &scan) && scan.ok;
}

static char *build_layout_dir_name(void) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == nullptr)
        return strutils_format("program");

    char *name = cwd;
    for (char *p = cwd; *p; p++) {
        if ((*p == '/' || *p == '\\') && p[1])
            name = p + 1;
    }

    word len = strlen(name);
    while (len > 0 && (name[len - 1] == '/' || name[len - 1] == '\\'))
        len--;
    return strutils_strndup(name, len);
}

bool build_layout_is_project(void) {
    return platform_dir_exists(BUILD_SRC_DIR) || project_file_exist();
}

/* The source group of src/ that the convention's executables link. Not a valid file stem,
   so it cannot clash with the name of an executable. */
#define BUILD_LAYOUT_OBJECTS "src/"

static bool build_layout_add_exe(build_layout_t *layout, build_source_t *source, const char *name,
                                 build_artifact_kind_t kind, const char *link) {
    build_target_t *target = build_targets_add(&layout->targets, name, BUILD_TARGET_EXECUTABLE);
    if (!target)
        return false;

    target->exe_kind = kind;
    if (link)
        list_add(&target->link, strutils_format("%s", link));
    return build_sources_add(&target->sources, source->path, source->lang);
}

/* The directory convention as targets:
   - with src/main.*: the other sources of src/ are a group of objects that the main
     executable, each src/bin/ executable and each test link directly;
   - without it: those sources are a library (static and shared), whose objects the
     executables and tests also link directly. */
static bool build_layout_convention_targets(build_layout_t *layout) {
    bool library = layout->main.count == 0;
    const char *link = nullptr;

    if (layout->lib.count) {
        build_target_t *target = build_targets_add(&layout->targets, library ? layout->name : BUILD_LAYOUT_OBJECTS,
                                                   library ? BUILD_TARGET_LIBRARY : BUILD_TARGET_OBJECTS);
        if (!target)
            return false;
        target->link_objects = true;
        for (word i = 0; i < layout->lib.count; i++) {
            if (!build_sources_add(&target->sources, layout->lib.items[i].path, layout->lib.items[i].lang))
                return false;
        }
        link = target->name;
    }

    if (layout->main.count && !build_layout_add_exe(layout, &layout->main.items[0], layout->name, BUILD_ARTIFACT_EXE, link))
        return false;
    for (word i = 0; i < layout->bins.count; i++) {
        if (!build_layout_add_exe(layout, &layout->bins.items[i], layout->bins.items[i].stem, BUILD_ARTIFACT_BIN, link))
            return false;
    }
    for (word i = 0; i < layout->tests.count; i++) {
        if (!build_layout_add_exe(layout, &layout->tests.items[i], layout->tests.items[i].stem, BUILD_ARTIFACT_TEST, link))
            return false;
    }

    // The convention's include path: src/ and, when it exists, include/.
    for (word t = 0; t < layout->targets.count; t++) {
        build_target_t *target = &layout->targets.items[t];
        list_add(&target->include_dirs, strutils_format("%s", BUILD_SRC_DIR));
        if (layout->has_include_dir)
            list_add(&target->include_dirs, strutils_format("%s", BUILD_INCLUDE_DIR));
    }
    return true;
}

static bool build_layout_load_convention(build_layout_t *layout, bool with_tests) {
    if (!platform_dir_exists(BUILD_SRC_DIR)) {
        log_error("Directory %s/ not found. The project sources must be in %s/.", BUILD_SRC_DIR, BUILD_SRC_DIR);
        return false;
    }

    if (!build_layout_scan(layout, BUILD_SRC_DIR))
        return false;

    if (with_tests && platform_dir_exists(BUILD_TESTS_DIR) && !build_layout_scan(layout, BUILD_TESTS_DIR))
        return false;

    layout->has_include_dir = platform_dir_exists(BUILD_INCLUDE_DIR);

    build_sources_sort(&layout->main);
    build_sources_sort(&layout->lib);
    build_sources_sort(&layout->bins);
    build_sources_sort(&layout->tests);

    if (layout->main.count > 1) {
        log_error("More than one main in %s/: %s and %s.", BUILD_SRC_DIR, layout->main.items[0].path, layout->main.items[1].path);
        return false;
    }

    if (layout->main.count + layout->lib.count + layout->bins.count == 0) {
        log_error("No C/C++ sources found in %s/.", BUILD_SRC_DIR);
        return false;
    }

    for (word i = 0; i < layout->bins.count; i++) {
        if (layout->main.count && strcmp(layout->bins.items[i].stem, layout->name) == 0) {
            log_error("%s produces an executable with the same name as the project (%s).", layout->bins.items[i].path, layout->name);
            return false;
        }
    }

    return build_layout_convention_targets(layout);
}

bool build_layout_load(build_layout_t *layout, bool with_tests) {
    RETURN_VAL_IF_FAIL(layout, false);

    memset(layout, 0, sizeof(build_layout_t));
    project_file_config_init(&layout->config);

    if (project_file_exist()) {
        if (!project_file_read(&layout->config))
            return false;
        layout->has_config = true;
    }

    if (layout->config.name && layout->config.name[0])
        layout->name = strutils_strndup(layout->config.name, strlen(layout->config.name));
    else
        layout->name = build_layout_dir_name();

    layout->has_targets = layout->config.targets.count > 0;
    bool loaded = layout->has_targets ? build_targets_from_config(&layout->targets, &layout->config)
                                      : build_layout_load_convention(layout, with_tests);
    return loaded && build_targets_resolve(&layout->targets);
}

void build_layout_clear(build_layout_t *layout) {
    RETURN_IF_FAIL(layout);

    free(layout->name);
    project_file_config_clean(&layout->config);
    build_sources_clear(&layout->main);
    build_sources_clear(&layout->lib);
    build_sources_clear(&layout->bins);
    build_sources_clear(&layout->tests);
    build_targets_clear(&layout->targets);
    memset(layout, 0, sizeof(build_layout_t));
}

bool build_layout_uses_lang(build_layout_t *layout, build_lang_t lang) {
    RETURN_VAL_IF_FAIL(layout, false);

    for (word t = 0; t < layout->targets.count; t++) {
        build_sources_t *sources = &layout->targets.items[t].sources;
        for (word i = 0; i < sources->count; i++) {
            if (sources->items[i].lang == lang)
                return true;
        }
    }
    return false;
}
