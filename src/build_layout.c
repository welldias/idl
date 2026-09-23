#include "build_layout.h"

typedef struct {
    build_layout_t *layout;
    const char *root;   // directory being scanned ("src" or "tests")
    bool ok;
} build_layout_scan_t;

static const struct {
    const char *ext;
    build_lang_t lang;
} build_source_exts[] = {
    { ".c", BUILD_LANG_C },
    { ".cpp", BUILD_LANG_CXX },
    { ".cc", BUILD_LANG_CXX },
    { ".cxx", BUILD_LANG_CXX },
    { ".c++", BUILD_LANG_CXX },
};

bool build_source_lang(const char *path, build_lang_t *lang) {
    RETURN_VAL_IF_FAIL(path, false);

    const char *dot = strrchr(path, '.');
    const char *slash = strrchr(path, '/');
    if (!dot || (slash && dot < slash))
        return false;

    for (word i = 0; i < SIZE_OF_ARRAY(build_source_exts); i++) {
        if (strcmp(dot, build_source_exts[i].ext) == 0) {
            if (lang)
                *lang = build_source_exts[i].lang;
            return true;
        }
    }
    return false;
}

static bool build_sources_add(build_sources_t *sources, const char *path, build_lang_t lang) {
    if (sources->count == sources->capacity) {
        word capacity = sources->capacity ? sources->capacity * 2 : 16;
        build_source_t *items = (build_source_t *)realloc(sources->items, capacity * sizeof(build_source_t));
        if (!items) {
            LOG_FATAL_NOT_ENOUGH_MEMORY();
            return false;
        }
        sources->items = items;
        sources->capacity = capacity;
    }

    const char *name = strrchr(path, '/');
    name = name ? name + 1 : path;
    const char *dot = strrchr(name, '.');

    build_source_t *source = &sources->items[sources->count++];
    source->path = strutils_strndup(path, strlen(path));
    source->stem = strutils_strndup(name, dot ? (int)(dot - name) : (int)strlen(name));
    source->lang = lang;
    return source->path && source->stem;
}

static void build_sources_clear(build_sources_t *sources) {
    for (word i = 0; i < sources->count; i++) {
        free(sources->items[i].path);
        free(sources->items[i].stem);
    }
    free(sources->items);
    memset(sources, 0, sizeof(build_sources_t));
}

static int build_source_cmp(const void *a, const void *b) {
    return strcmp(((const build_source_t *)a)->path, ((const build_source_t *)b)->path);
}

static void build_sources_sort(build_sources_t *sources) {
    if (sources->count > 1)
        qsort(sources->items, sources->count, sizeof(build_source_t), build_source_cmp);
}

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

    return true;
}

void build_layout_clear(build_layout_t *layout) {
    RETURN_IF_FAIL(layout);

    free(layout->name);
    project_file_config_clean(&layout->config);
    build_sources_clear(&layout->main);
    build_sources_clear(&layout->lib);
    build_sources_clear(&layout->bins);
    build_sources_clear(&layout->tests);
    memset(layout, 0, sizeof(build_layout_t));
}

bool build_layout_uses_lang(build_layout_t *layout, build_lang_t lang, bool with_tests) {
    build_sources_t *groups[] = { &layout->main, &layout->lib, &layout->bins, &layout->tests };
    word group_count = with_tests ? 4 : 3;

    for (word g = 0; g < group_count; g++) {
        for (word i = 0; i < groups[g]->count; i++) {
            if (groups[g]->items[i].lang == lang)
                return true;
        }
    }
    return false;
}
