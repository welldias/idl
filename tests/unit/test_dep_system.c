#include "idl_test.h"
#include "dep_system.h"

static void test_builtin(void) {
    dep_system_t dep;
    CHECK(dep_system_find("m", &dep));
    CHECK_INT(dep.kind, DEP_SYSTEM_BUILTIN);
    CHECK(idl_test_list_has(&dep.ldflags, "-lm"));
    char *where = dep_system_describe(&dep);
    CHECK_STR(where, "system library");
    free(where);
    dep_system_clear(&dep);

    CHECK(dep_system_find("pthread", &dep));
    CHECK(idl_test_list_has(&dep.cflags, "-pthread"));
    CHECK(idl_test_list_has(&dep.ldflags, "-pthread"));
    dep_system_clear(&dep);
}

#if !defined(_WIN32)
static void test_library_dir(void) {
    idl_test_enter_dir("libs");
    idl_test_write("prefix/lib/libidlfake.a", "");
    idl_test_write("prefix/include/idlfake.h", "");
    idl_test_write("prefix/lib/libidlversioned.so.1", ""); // not usable by -lidlversioned
    idl_test_write("other/libidlother.a", "");

    char cwd[1024];
    CHECK(getcwd(cwd, sizeof(cwd)) != nullptr);
    char *path = strutils_format("%s/nowhere:%s/prefix/lib:%s/other", cwd, cwd, cwd);
    idl_test_setenv("LD_LIBRARY_PATH", path);
    idl_test_setenv("DYLD_LIBRARY_PATH", nullptr);
    free(path);

    dep_system_t dep;
    CHECK(dep_system_find("idlfake", &dep));
    CHECK_INT(dep.kind, DEP_SYSTEM_LIBRARY);
    CHECK_STR(dep.name, "idlfake");
    char *file = strutils_format("%s/prefix/lib/libidlfake.a", cwd);
    char *lib_dir = strutils_format("-L%s/prefix/lib", cwd);
    char *include = strutils_format("-I%s/prefix/lib/../include", cwd);
    CHECK_STR(dep.file, file);
    CHECK(idl_test_list_has(&dep.ldflags, lib_dir));
    CHECK(idl_test_list_has(&dep.ldflags, "-lidlfake"));
    CHECK(idl_test_list_has(&dep.cflags, include));
    char *where = dep_system_describe(&dep);
    CHECK_STR(where, file);
    free(where);
    dep_system_clear(&dep);

    // "libidlfake" is the library idlfake.
    CHECK(dep_system_find("libidlfake", &dep));
    CHECK_STR(dep.name, "idlfake");
    CHECK(idl_test_list_has(&dep.ldflags, "-lidlfake"));
    dep_system_clear(&dep);

    // No include/ next to other/: no -I.
    CHECK(dep_system_find("idlother", &dep));
    CHECK_INT(dep.cflags.count, 0);
    dep_system_clear(&dep);

    CHECK(!dep_system_find("idlversioned", &dep));
    dep_system_clear(&dep);

    free(file);
    free(lib_dir);
    free(include);
    idl_test_setenv("LD_LIBRARY_PATH", nullptr);
}
#endif

static void test_not_found(void) {
    dep_system_t dep;
    CHECK(!dep_system_find("idl-missing-lib", &dep));
    dep_system_clear(&dep);
    CHECK(!dep_system_find("", &dep));
    dep_system_clear(&dep);
}

/* pkg-config, when installed: a .pc file in a temporary PKG_CONFIG_PATH. */
static void test_pkg_config(void) {
    idl_test_enter_dir("pc");
    idl_test_write("idlfakepc.pc",
        "Name: idlfakepc\n"
        "Description: fake package for the idl tests\n"
        "Version: 4.5.6\n"
        "Cflags: -I/opt/idlfakepc/include -DIDLFAKEPC\n"
        "Libs: -L/opt/idlfakepc/lib -lidlfakepc\n");

    char cwd[1024];
    CHECK(getcwd(cwd, sizeof(cwd)) != nullptr);
    idl_test_setenv("PKG_CONFIG_PATH", cwd);

    dep_system_t dep;
    if (!dep_system_find("idlfakepc", &dep)) {
        printf("pkg-config not available: part of the test skipped\n");
        dep_system_clear(&dep);
        return;
    }
    CHECK_INT(dep.kind, DEP_SYSTEM_PKG_CONFIG);
    CHECK_STR(dep.version, "4.5.6");
    CHECK(idl_test_list_has(&dep.cflags, "-DIDLFAKEPC"));
    CHECK(idl_test_list_has(&dep.cflags, "-I/opt/idlfakepc/include"));
    CHECK(idl_test_list_has(&dep.ldflags, "-lidlfakepc"));
    char *where = dep_system_describe(&dep);
    CHECK_STR(where, "pkg-config, version 4.5.6");
    free(where);
    dep_system_clear(&dep);
}

int main(void) {
    RUN_TEST(test_builtin);
#if !defined(_WIN32)
    RUN_TEST(test_library_dir);
#endif
    RUN_TEST(test_not_found);
    RUN_TEST(test_pkg_config);
    return idl_test_report();
}
