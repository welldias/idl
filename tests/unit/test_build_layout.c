#include "idl_test.h"
#include "build_layout.h"

static void test_source_lang(void) {
    build_lang_t lang;
    CHECK(build_source_lang("src/a.c", &lang));
    CHECK_INT(lang, BUILD_LANG_C);
    CHECK(build_source_lang("src/a.cpp", &lang));
    CHECK_INT(lang, BUILD_LANG_CXX);
    CHECK(build_source_lang("a.cc", &lang) && lang == BUILD_LANG_CXX);
    CHECK(build_source_lang("a.cxx", &lang) && lang == BUILD_LANG_CXX);
    CHECK(build_source_lang("a.c++", &lang) && lang == BUILD_LANG_CXX);

    CHECK(!build_source_lang("src/a.h", &lang));
    CHECK(!build_source_lang("src/a.hpp", &lang));
    CHECK(!build_source_lang("Makefile", &lang));
    CHECK(!build_source_lang("dir.c/file", &lang));
}

static void test_source_for_os(void) {
    unsigned on_linux = BUILD_OS_LINUX | BUILD_OS_UNIX;
    unsigned on_macos = BUILD_OS_MACOS | BUILD_OS_UNIX;
    unsigned on_windows = BUILD_OS_WINDOWS;

    CHECK(build_source_for_os("src/code.c", on_windows));
    CHECK(build_source_for_os("src/code.c", on_linux));

    CHECK(build_source_for_os("src/code_win.c", on_windows));
    CHECK(!build_source_for_os("src/code_win.c", on_linux));
    CHECK(!build_source_for_os("src/code_win.c", on_macos));

    CHECK(build_source_for_os("src/sub/code_linux.c", on_linux));
    CHECK(!build_source_for_os("src/sub/code_linux.c", on_macos));
    CHECK(!build_source_for_os("src/sub/code_linux.c", on_windows));

    CHECK(build_source_for_os("src/code_macos.cpp", on_macos));
    CHECK(!build_source_for_os("src/code_macos.cpp", on_linux));
    CHECK(!build_source_for_os("src/code_macos.cpp", on_windows));

    CHECK(build_source_for_os("src/code_unix.cpp", on_linux));
    CHECK(build_source_for_os("src/code_unix.cpp", on_macos));
    CHECK(build_source_for_os("src/code_unix.cpp", BUILD_OS_UNIX)); // e.g. FreeBSD
    CHECK(!build_source_for_os("src/code_unix.cpp", on_windows));

    // Only the suffix of the file name counts.
    CHECK(build_source_for_os("src/win.c", on_linux));
    CHECK(build_source_for_os("src/_win.c", on_linux));
    CHECK(build_source_for_os("src/code_windows.c", on_linux));
    CHECK(build_source_for_os("src/code_win/other.c", on_linux));
    CHECK(build_source_for_os("src/code_Win.c", on_linux));

    unsigned host = build_os_host();
    CHECK(host != 0);
#if defined(_WIN32)
    CHECK_INT(host, BUILD_OS_WINDOWS);
#elif defined(__APPLE__)
    CHECK_INT(host, BUILD_OS_MACOS | BUILD_OS_UNIX);
#elif defined(__linux__)
    CHECK_INT(host, BUILD_OS_LINUX | BUILD_OS_UNIX);
#endif
}

static void test_platform_sources(void) {
    idl_test_enter_dir("platform_sources");
    idl_test_write("src/main.c", "");
    idl_test_write("src/io_win.c", "");
    idl_test_write("src/io_linux.c", "");
    idl_test_write("src/io_macos.c", "");
    idl_test_write("src/io_unix.cpp", "");
    idl_test_write("src/bin/tool_win.c", "");
    idl_test_write("tests/test_io_unix.c", "");

    build_layout_t layout;
    CHECK(build_layout_load(&layout, true));
#if defined(_WIN32)
    CHECK_INT(layout.lib.count, 1);
    CHECK_STR(layout.lib.items[0].path, "src/io_win.c");
    CHECK_INT(layout.bins.count, 1);
    CHECK_INT(layout.tests.count, 0);
#elif defined(__APPLE__)
    CHECK_INT(layout.lib.count, 2);
    CHECK_STR(layout.lib.items[0].path, "src/io_macos.c");
    CHECK_STR(layout.lib.items[1].path, "src/io_unix.cpp");
    CHECK_INT(layout.bins.count, 0);
    CHECK_INT(layout.tests.count, 1);
#elif defined(__linux__)
    CHECK_INT(layout.lib.count, 2);
    CHECK_STR(layout.lib.items[0].path, "src/io_linux.c");
    CHECK_STR(layout.lib.items[1].path, "src/io_unix.cpp");
    CHECK_INT(layout.bins.count, 0);
    CHECK_INT(layout.tests.count, 1);
#endif
    build_layout_clear(&layout);
}

static void write_full_project(void) {
    idl_test_write("src/main.c", "");
    idl_test_write("src/zeta.c", "");
    idl_test_write("src/sub/alpha.cpp", "");
    idl_test_write("src/util.h", "");
    idl_test_write("src/notes.txt", "");
    idl_test_write("src/bin/tool.c", "");
    idl_test_write("src/bin/deep/ignored.c", "");
    idl_test_write("tests/test_one.c", "");
    idl_test_write("tests/helpers/ignored.c", "");
    idl_test_write("include/api.h", "");
}

static void test_classification(void) {
    idl_test_enter_dir("demo_layout");
    write_full_project();

    build_layout_t layout;
    CHECK(build_layout_load(&layout, true));
    CHECK_STR(layout.name, "demo_layout"); // no project.yml: directory name
    CHECK(!layout.has_config);
    CHECK(layout.has_include_dir);

    CHECK_INT(layout.main.count, 1);
    CHECK_STR(layout.main.items[0].path, "src/main.c");

    CHECK_INT(layout.lib.count, 2); // sorted by path
    CHECK_STR(layout.lib.items[0].path, "src/sub/alpha.cpp");
    CHECK_STR(layout.lib.items[0].stem, "alpha");
    CHECK_INT(layout.lib.items[0].lang, BUILD_LANG_CXX);
    CHECK_STR(layout.lib.items[1].path, "src/zeta.c");

    CHECK_INT(layout.bins.count, 1);
    CHECK_STR(layout.bins.items[0].stem, "tool");

    CHECK_INT(layout.tests.count, 1);
    CHECK_STR(layout.tests.items[0].stem, "test_one");

    CHECK(build_layout_uses_lang(&layout, BUILD_LANG_C, true));
    CHECK(build_layout_uses_lang(&layout, BUILD_LANG_CXX, true));

    build_layout_clear(&layout);
}

static void test_without_tests(void) {
    idl_test_enter_dir("no_tests");
    write_full_project();
    idl_test_write("tests/test_cpp.cpp", "");
    platform_remove_tree("src/sub");

    build_layout_t layout;
    CHECK(build_layout_load(&layout, false));
    CHECK_INT(layout.tests.count, 0);
    CHECK(!build_layout_uses_lang(&layout, BUILD_LANG_CXX, false));
    build_layout_clear(&layout);
}

static void test_name_from_config(void) {
    idl_test_enter_dir("config_name");
    idl_test_write("src/main.c", "");
    idl_test_write(PROJECT_FILE_NAME, "project:\n  name: other-name\n  requires-c: C11\n");

    build_layout_t layout;
    CHECK(build_layout_load(&layout, false));
    CHECK(layout.has_config);
    CHECK_STR(layout.name, "other-name");
    CHECK_STR(layout.config.requires_c, "C11");
    CHECK(!layout.has_include_dir);
    build_layout_clear(&layout);
}

static void check_load_fails(const char *dir) {
    build_layout_t layout;
    CHECK(!build_layout_load(&layout, false));
    build_layout_clear(&layout);
    (void)dir;
}

static void test_errors(void) {
    idl_test_enter_dir("no_src");
    CHECK(!build_layout_is_project());
    check_load_fails("no_src");

    idl_test_enter_dir("empty_src");
    platform_make_dirs("src");
    CHECK(build_layout_is_project());
    check_load_fails("empty_src");

    idl_test_enter_dir("two_mains");
    idl_test_write("src/main.c", "");
    idl_test_write("src/main.cpp", "");
    check_load_fails("two_mains");

    idl_test_enter_dir("bin_clash");
    idl_test_write("src/main.c", "");
    idl_test_write("src/bin/bin_clash.c", "");
    check_load_fails("bin_clash");

    idl_test_enter_dir("bad_config");
    idl_test_write("src/main.c", "");
    idl_test_write(PROJECT_FILE_NAME, "project: [\n");
    check_load_fails("bad_config");
}

static void test_library_only(void) {
    idl_test_enter_dir("lib_only");
    idl_test_write("src/a.c", "");
    idl_test_write("src/bin/cli.c", "");

    build_layout_t layout;
    CHECK(build_layout_load(&layout, false));
    CHECK_INT(layout.main.count, 0);
    CHECK_INT(layout.lib.count, 1);
    CHECK_INT(layout.bins.count, 1);
    build_layout_clear(&layout);
}

int main(void) {
    RUN_TEST(test_source_lang);
    RUN_TEST(test_source_for_os);
    RUN_TEST(test_platform_sources);
    RUN_TEST(test_classification);
    RUN_TEST(test_without_tests);
    RUN_TEST(test_name_from_config);
    RUN_TEST(test_errors);
    RUN_TEST(test_library_only);
    return idl_test_report();
}
