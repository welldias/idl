#include "idl_test.h"
#include "build_glob.h"

static void test_match(void) {
    CHECK(build_glob_match("src/main.c", "src/main.c"));
    CHECK(!build_glob_match("src/main.c", "src/main.cpp"));

    CHECK(build_glob_match("src/*.c", "src/a.c"));
    CHECK(!build_glob_match("src/*.c", "src/sub/a.c")); // '*' does not cross '/'
    CHECK(!build_glob_match("src/*.c", "src/a.cpp"));
    CHECK(build_glob_match("*.c", "a.c"));
    CHECK(!build_glob_match("*.c", "dir/a.c"));
    CHECK(build_glob_match("src/a*b.c", "src/ab.c"));
    CHECK(build_glob_match("src/a*b.c", "src/a_x_b.c"));

    CHECK(build_glob_match("src/?.c", "src/a.c"));
    CHECK(!build_glob_match("src/?.c", "src/ab.c"));
    CHECK(!build_glob_match("src?a.c", "src/a.c"));

    CHECK(build_glob_match("src/**/*.c", "src/a.c")); // "**/" also matches no directory
    CHECK(build_glob_match("src/**/*.c", "src/x/a.c"));
    CHECK(build_glob_match("src/**/*.c", "src/x/y/z/a.c"));
    CHECK(!build_glob_match("src/**/*.c", "lib/a.c"));
    CHECK(build_glob_match("src/**", "src/x/y.c"));
    CHECK(build_glob_match("**/*.cpp", "a.cpp"));
    CHECK(build_glob_match("**/*.cpp", "x/y/a.cpp"));
    CHECK(build_glob_match("lib/**/test_*.c", "lib/a/b/test_one.c"));
    CHECK(!build_glob_match("lib/**/test_*.c", "lib/a/b/one.c"));
}

static void write_tree(void) {
    idl_test_write("lib/a.c", "");
    idl_test_write("lib/b.cpp", "");
    idl_test_write("lib/b.h", "");
    idl_test_write("lib/io_win.c", "");
    idl_test_write("lib/io_unix.c", "");
    idl_test_write("lib/legacy/old.c", "");
    idl_test_write("lib/deep/x/y.c", "");
    idl_test_write("apps/main.c", "");
    idl_test_write("top.c", "");
    idl_test_write(".hidden/h.c", "");
    idl_test_write("build/debug/gen.c", "");
}

static void add(list_t *list, const char *value) {
    list_add(list, strdup(value));
}

static void test_expand(void) {
    idl_test_enter_dir("expand");
    write_tree();

    list_t patterns = {0};
    list_t excludes = {0};
    list_init(&patterns, free);
    list_init(&excludes, free);
    add(&patterns, "lib/**/*.c");
    add(&patterns, "lib/*.cpp");
    add(&patterns, "./apps/main.c");
    add(&patterns, "lib/a.c"); // already matched: not repeated
    add(&excludes, "lib/legacy/**");

    build_sources_t out = {0};
    CHECK(build_glob_expand(&patterns, &excludes, "target 'x'", &out));

#if defined(_WIN32)
    const char *os_file = "lib/io_win.c";
#else
    const char *os_file = "lib/io_unix.c";
#endif
    // Sorted by path; sources of other systems skipped.
    const char *expected[] = { "apps/main.c", "lib/a.c", "lib/b.cpp", "lib/deep/x/y.c", os_file };
    CHECK_INT(out.count, SIZE_OF_ARRAY(expected));
    for (word i = 0; i < out.count && i < SIZE_OF_ARRAY(expected); i++)
        CHECK_STR(out.items[i].path, expected[i]);
    CHECK(build_sources_contains(&out, "lib/b.cpp"));
    CHECK(!build_sources_contains(&out, "lib/legacy/old.c"));
    build_sources_clear(&out);

    // A glob from the project root skips hidden directories and build/.
    list_clear(&patterns);
    add(&patterns, "**/*.c");
    CHECK(build_glob_expand(&patterns, nullptr, "target 'x'", &out));
    CHECK(build_sources_contains(&out, "top.c"));
    CHECK(build_sources_contains(&out, "lib/legacy/old.c"));
    CHECK(!build_sources_contains(&out, ".hidden/h.c"));
    CHECK(!build_sources_contains(&out, "build/debug/gen.c"));
    build_sources_clear(&out);

    // A glob that matches nothing is only a warning.
    list_clear(&patterns);
    add(&patterns, "nothing/**/*.c");
    CHECK(build_glob_expand(&patterns, nullptr, "target 'x'", &out));
    CHECK_INT(out.count, 0);

    list_clear(&patterns);
    list_clear(&excludes);
}

static void check_expand_fails(const char *pattern) {
    list_t patterns = {0};
    list_init(&patterns, free);
    add(&patterns, pattern);

    build_sources_t out = {0};
    CHECK(!build_glob_expand(&patterns, nullptr, "target 'x'", &out));
    build_sources_clear(&out);
    list_clear(&patterns);
}

static void test_expand_errors(void) {
    idl_test_enter_dir("errors");
    write_tree();

    check_expand_fails("lib/missing.c");
    check_expand_fails("lib/b.h");
    check_expand_fails("../outside.c");
    check_expand_fails("lib/../../outside.c");
    check_expand_fails("/usr/src/a.c");
}

int main(void) {
    RUN_TEST(test_match);
    RUN_TEST(test_expand);
    RUN_TEST(test_expand_errors);
    return idl_test_report();
}
