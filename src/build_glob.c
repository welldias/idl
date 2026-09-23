#include "build_glob.h"

bool build_glob_match(const char *pattern, const char *path) {
    RETURN_VAL_IF_FAIL(pattern, false);
    RETURN_VAL_IF_FAIL(path, false);

    const char *p = pattern;
    const char *s = path;

    while (*p) {
        if (p[0] == '*' && p[1] == '*') {
            const char *rest = p + 2;
            bool dirs = *rest == '/'; // "**/": zero or more whole directories
            if (dirs)
                rest++;

            if (build_glob_match(rest, s))
                return true;
            for (const char *t = s; *t; t++) {
                if ((!dirs || *t == '/') && build_glob_match(rest, t + 1))
                    return true;
            }
            return false;
        }

        if (*p == '*') {
            for (const char *t = s;; t++) {
                if (build_glob_match(p + 1, t))
                    return true;
                if (!*t || *t == '/')
                    return false;
            }
        }

        if (!*s)
            return false;
        if (*p == '?' ? *s == '/' : *p != *s)
            return false;
        p++;
        s++;
    }
    return *s == '\0';
}

static bool build_glob_has_wildcard(const char *pattern) {
    return strpbrk(pattern, "*?") != nullptr;
}

/* Removes "./" prefixes and turns '\' into '/'. Returns NULL for paths outside the project. */
static char *build_glob_normalize(const char *pattern) {
    char *path = strutils_strndup(pattern, (int)strlen(pattern));
    if (!path)
        return nullptr;

    for (char *c = path; *c; c++) {
        if (*c == '\\')
            *c = '/';
    }

    char *start = path;
    while (start[0] == '.' && start[1] == '/')
        start += 2;

    bool outside = start[0] == '/' || start[0] == '\0' || (start[0] && start[1] == ':') ||
                   strcmp(start, "..") == 0 || strncmp(start, "../", 3) == 0 || strstr(start, "/../") ||
                   (strlen(start) >= 3 && strcmp(start + strlen(start) - 3, "/..") == 0);
    if (outside) {
        free(path);
        return nullptr;
    }

    memmove(path, start, strlen(start) + 1);
    return path;
}

typedef struct {
    const char *pattern;
    build_sources_t *out;
    word matched;
    bool ok;
} build_glob_scan_t;

static void build_glob_on_file(const char *full_path, void *arg) {
    build_glob_scan_t *scan = (build_glob_scan_t *)arg;

    char path[1024];
    snprintf(path, sizeof(path), "%s", full_path);
    for (char *c = path; *c; c++) {
        if (*c == '\\')
            *c = '/';
    }
    const char *rel = strncmp(path, "./", 2) == 0 ? path + 2 : path;

    // Wildcards never enter hidden directories (.git, .cache...) or the build output.
    if (rel[0] == '.' || strstr(rel, "/.") || (strncmp(rel, "build/", 6) == 0 && strncmp(scan->pattern, "build/", 6) != 0))
        return;

    build_lang_t lang;
    if (!build_glob_match(scan->pattern, rel) || !build_source_lang(rel, &lang))
        return;

    scan->matched++;
    if (!build_source_for_os(rel, build_os_host()) || build_sources_contains(scan->out, rel))
        return;
    if (!build_sources_add(scan->out, rel, lang))
        scan->ok = false;
}

static bool build_glob_expand_one(const char *pattern, const char *owner, build_sources_t *out) {
    char *path = build_glob_normalize(pattern);
    if (!path) {
        log_error("%s: '%s' is outside the project directory.", owner, pattern);
        return false;
    }

    bool result = true;
    if (!build_glob_has_wildcard(path)) {
        build_lang_t lang;
        if (!platform_file_exists(path)) {
            log_error("%s: source '%s' not found.", owner, path);
            result = false;
        } else if (!build_source_lang(path, &lang)) {
            log_error("%s: '%s' is not a C/C++ source (.c, .cpp, .cc, .cxx, .c++).", owner, path);
            result = false;
        } else if (build_source_for_os(path, build_os_host()) && !build_sources_contains(out, path)) {
            result = build_sources_add(out, path, lang);
        }
        free(path);
        return result;
    }

    // Only the directories before the first wildcard need to be scanned.
    char *base = strutils_strndup(path, (int)strlen(path));
    char *slash = nullptr;
    for (char *c = base; *c && *c != '*' && *c != '?'; c++) {
        if (*c == '/')
            slash = c;
    }
    if (slash)
        *slash = '\0';
    else
        strcpy(base, ".");

    build_glob_scan_t scan = { .pattern = path, .out = out, .ok = true };
    if (platform_dir_exists(base))
        platform_scan_directory(base, nullptr, build_glob_on_file, &scan);

    if (scan.matched == 0)
        log_warn("%s: '%s' matched no sources.", owner, path);

    result = scan.ok;
    free(base);
    free(path);
    return result;
}

bool build_glob_expand(list_t *patterns, list_t *excludes, const char *owner, build_sources_t *out) {
    RETURN_VAL_IF_FAIL(patterns, false);
    RETURN_VAL_IF_FAIL(out, false);

    for (list_item_t *item = patterns->head; item; item = item->next) {
        if (!build_glob_expand_one((const char *)item->value, owner, out))
            return false;
    }

    for (list_item_t *item = excludes ? excludes->head : nullptr; item; item = item->next) {
        char *pattern = build_glob_normalize((const char *)item->value);
        if (!pattern) {
            log_error("%s: '%s' is outside the project directory.", owner, (const char *)item->value);
            return false;
        }

        word kept = 0;
        for (word i = 0; i < out->count; i++) {
            if (build_glob_match(pattern, out->items[i].path)) {
                free(out->items[i].path);
                free(out->items[i].stem);
            } else {
                out->items[kept++] = out->items[i];
            }
        }
        out->count = kept;
        free(pattern);
    }

    build_sources_sort(out);
    return true;
}
