#include "build_source.h"

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

static const struct {
    const char *suffix;
    unsigned os;
} build_os_suffixes[] = {
    { "_win", BUILD_OS_WINDOWS },
    { "_linux", BUILD_OS_LINUX },
    { "_macos", BUILD_OS_MACOS },
    { "_unix", BUILD_OS_UNIX },
};

unsigned build_os_host(void) {
#if defined(_WIN32)
    return BUILD_OS_WINDOWS;
#elif defined(__APPLE__)
    return BUILD_OS_MACOS | BUILD_OS_UNIX;
#elif defined(__linux__)
    return BUILD_OS_LINUX | BUILD_OS_UNIX;
#else
    return BUILD_OS_UNIX;
#endif
}

bool build_source_for_os(const char *path, unsigned os) {
    RETURN_VAL_IF_FAIL(path, false);

    const char *name = strrchr(path, '/');
    name = name ? name + 1 : path;
    const char *dot = strrchr(name, '.');
    word stem_len = dot ? (word)(dot - name) : strlen(name);

    for (word i = 0; i < SIZE_OF_ARRAY(build_os_suffixes); i++) {
        word suffix_len = strlen(build_os_suffixes[i].suffix);
        if (stem_len > suffix_len && strncmp(name + stem_len - suffix_len, build_os_suffixes[i].suffix, suffix_len) == 0)
            return (os & build_os_suffixes[i].os) != 0;
    }
    return true;
}

bool build_sources_add(build_sources_t *sources, const char *path, build_lang_t lang) {
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

void build_sources_clear(build_sources_t *sources) {
    for (word i = 0; i < sources->count; i++) {
        free(sources->items[i].path);
        free(sources->items[i].stem);
    }
    free(sources->items);
    memset(sources, 0, sizeof(build_sources_t));
}

bool build_sources_contains(build_sources_t *sources, const char *path) {
    for (word i = 0; i < sources->count; i++) {
        if (strcmp(sources->items[i].path, path) == 0)
            return true;
    }
    return false;
}

static int build_source_cmp(const void *a, const void *b) {
    return strcmp(((const build_source_t *)a)->path, ((const build_source_t *)b)->path);
}

void build_sources_sort(build_sources_t *sources) {
    if (sources->count > 1)
        qsort(sources->items, sources->count, sizeof(build_source_t), build_source_cmp);
}
