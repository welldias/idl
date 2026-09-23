#include "idl_test.h"
#include "build_toolchain.h"

static void test_env_override(void) {
    idl_test_setenv("CC", "meu-cc");
    idl_test_setenv("CXX", "meu-cxx");
    idl_test_setenv("AR", "meu-ar");

    build_toolchain_t toolchain;
    CHECK(build_toolchain_init(&toolchain, true, true));
    CHECK_STR(toolchain.cc, "meu-cc");
    CHECK_STR(toolchain.cxx, "meu-cxx");
    CHECK_STR(toolchain.ar, "meu-ar");
    build_toolchain_clear(&toolchain);

    idl_test_setenv("CC", nullptr);
    idl_test_setenv("CXX", nullptr);
    idl_test_setenv("AR", nullptr);
}

static void test_only_needed_compilers(void) {
    idl_test_setenv("CC", "meu-cc");

    build_toolchain_t toolchain;
    CHECK(build_toolchain_init(&toolchain, true, false));
    CHECK_STR(toolchain.cc, "meu-cc");
    CHECK(toolchain.cxx == nullptr);
    CHECK_STR(toolchain.ar, "ar");
    build_toolchain_clear(&toolchain);

    idl_test_setenv("CC", nullptr);
}

static void test_no_compiler_in_path(void) {
    idl_test_enter_dir("empty_path");
    char *old_path = getenv("PATH") ? strdup(getenv("PATH")) : nullptr;
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    idl_test_setenv("PATH", cwd);

    build_toolchain_t toolchain;
    CHECK(!build_toolchain_init(&toolchain, true, false));
    build_toolchain_clear(&toolchain);

    CHECK(!build_toolchain_init(&toolchain, false, true));
    build_toolchain_clear(&toolchain);

    CHECK(build_toolchain_init(&toolchain, false, false)); // nada é necessário
    build_toolchain_clear(&toolchain);

    idl_test_setenv("PATH", old_path);
    free(old_path);
}

static void test_resolve_deps(void) {
    list_t deps = {0};
    list_init(&deps, free);
    list_add(&deps, strdup("m"));
    list_add(&deps, strdup("pthread"));
    list_add(&deps, strdup("dl"));
    list_add(&deps, strdup("idl-lib-que-nao-existe"));

    build_toolchain_t toolchain;
    build_toolchain_init(&toolchain, false, false);
    CHECK(build_toolchain_resolve_deps(&toolchain, &deps));

    CHECK(idl_test_list_has(&toolchain.ldflags, "-lm"));
    CHECK(idl_test_list_has(&toolchain.ldflags, "-ldl"));
    CHECK(idl_test_list_has(&toolchain.ldflags, "-pthread"));
    CHECK(idl_test_list_has(&toolchain.cflags, "-pthread"));
    // Desconhecida pelo pkg-config (ou sem pkg-config): usa -l<nome>.
    CHECK(idl_test_list_has(&toolchain.ldflags, "-lidl-lib-que-nao-existe"));

    build_toolchain_clear(&toolchain);
    list_clear(&deps);
}

int main(void) {
    RUN_TEST(test_env_override);
    RUN_TEST(test_only_needed_compilers);
    RUN_TEST(test_no_compiler_in_path);
    RUN_TEST(test_resolve_deps);
    return idl_test_report();
}
