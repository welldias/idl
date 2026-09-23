/* Tiny unit test framework for idl.
 *
 * Each test_<module>.c file declares test functions and runs them with RUN_TEST
 * inside main; the exit code is 0 only if every check passes. */
#ifndef IDL_TEST_H
#define IDL_TEST_H

#include "idl.h"

#include <uv.h>

static int idl_test_checks = 0;
static int idl_test_failures = 0;

#define CHECK(expr)                                                                  \
    do {                                                                             \
        idl_test_checks++;                                                           \
        if (!(expr)) {                                                               \
            fprintf(stderr, "%s:%d: failed: %s\n", __FILE__, __LINE__, #expr);       \
            idl_test_failures++;                                                     \
        }                                                                            \
    } while (0)

#define CHECK_STR(actual, expected)                                                  \
    do {                                                                             \
        const char *actual_ = (actual);                                              \
        const char *expected_ = (expected);                                          \
        idl_test_checks++;                                                           \
        if (!actual_ || strcmp(actual_, expected_) != 0) {                           \
            fprintf(stderr, "%s:%d: %s: expected \"%s\", got \"%s\"\n", __FILE__, \
                    __LINE__, #actual, expected_, actual_ ? actual_ : "(null)");     \
            idl_test_failures++;                                                     \
        }                                                                            \
    } while (0)

#define CHECK_INT(actual, expected)                                                  \
    do {                                                                             \
        long long actual_ = (long long)(actual);                                     \
        long long expected_ = (long long)(expected);                                 \
        idl_test_checks++;                                                           \
        if (actual_ != expected_) {                                                  \
            fprintf(stderr, "%s:%d: %s: expected %lld, got %lld\n", __FILE__,     \
                    __LINE__, #actual, expected_, actual_);                          \
            idl_test_failures++;                                                     \
        }                                                                            \
    } while (0)

#define RUN_TEST(fn)                                                                 \
    do {                                                                             \
        int failures_before_ = idl_test_failures;                                    \
        fn();                                                                        \
        printf("[%s] %s\n", idl_test_failures == failures_before_ ? " OK " : "FAIL", \
               #fn);                                                                 \
    } while (0)

static inline int idl_test_report(void) {
    printf("%d checks, %d failures\n", idl_test_checks, idl_test_failures);
    return idl_test_failures ? 1 : 0;
}

/* Recreates IDL_TEST_WORK_DIR/<name> empty and enters it. */
static inline void idl_test_enter_dir(const char *name) {
    char *dir = strutils_format("%s/%s", IDL_TEST_WORK_DIR, name);
    platform_remove_tree(dir);
    platform_make_dirs(dir);
    if (uv_chdir(dir) != 0) {
        fprintf(stderr, "could not enter %s\n", dir);
        exit(2);
    }
    free(dir);
}

/* Writes a file, creating its parent directories. */
static inline void idl_test_write(const char *path, const char *content) {
    const char *slash = strrchr(path, '/');
    if (slash) {
        char *dir = strutils_strndup(path, (int)(slash - path));
        platform_make_dirs(dir);
        free(dir);
    }

    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "could not write %s\n", path);
        exit(2);
    }
    fputs(content, f);
    fclose(f);
}

/* Reads a whole file (release with free). Returns NULL if it does not exist. */
static inline char *idl_test_read(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return nullptr;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char *content = (char *)calloc(size + 1, 1);
    if (content)
        fread(content, 1, size, f);
    fclose(f);
    return content;
}

static inline void idl_test_setenv(const char *name, const char *value) {
#ifdef _WIN32
    _putenv_s(name, value ? value : "");
#else
    if (value)
        setenv(name, value, 1);
    else
        unsetenv(name);
#endif
}

static inline bool idl_test_list_has(list_t *list, const char *value) {
    for (list_item_t *item = list->head; item; item = item->next) {
        if (strcmp((const char *)item->value, value) == 0)
            return true;
    }
    return false;
}

#endif // IDL_TEST_H
