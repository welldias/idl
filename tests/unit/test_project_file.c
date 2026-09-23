#include "idl_test.h"
#include "project_file.h"

static void test_missing_file(void) {
    idl_test_enter_dir("missing");

    CHECK(!project_file_exist());

    project_config_t config = {0};
    project_file_config_init(&config);
    CHECK(!project_file_read(&config));
    project_file_config_clean(&config);
}

static void test_save_and_read_basic(void) {
    idl_test_enter_dir("basic");

    project_config_t config = {0};
    project_file_config_init(&config);
    project_file_name_set(&config, "demo");
    project_file_version_set(&config, "0.1.0");
    project_file_description_set(&config, "");
    project_file_requires_c_set(&config, "C23");
    CHECK(project_file_save(&config));
    project_file_config_clean(&config);

    CHECK(project_file_exist());

    // Sem requires-cpp nem listas em build, essas chaves não aparecem no arquivo.
    char *text = idl_test_read(PROJECT_FILE_NAME);
    CHECK(text && strstr(text, "project:"));
    CHECK(text && !strstr(text, "requires-cpp"));
    CHECK(text && !strstr(text, "build:"));
    free(text);

    project_file_config_init(&config);
    CHECK(project_file_read(&config));
    CHECK_STR(config.name, "demo");
    CHECK_STR(config.version, "0.1.0");
    CHECK_STR(config.requires_c, "C23");
    CHECK(config.requires_cpp == nullptr);
    CHECK_INT(config.dependencies.count, 0);
    project_file_config_clean(&config);

    CHECK(config.name == nullptr);
    CHECK(config.requires_c == nullptr);
}

static void test_full_roundtrip(void) {
    idl_test_enter_dir("full");

    project_config_t config = {0};
    project_file_config_init(&config);
    project_file_name_set(&config, "nome: \"estranho\"");
    project_file_version_set(&config, "1.0");
    project_file_description_set(&config, "linha1\nlinha2");
    project_file_requires_c_set(&config, "C17");
    project_file_requires_cpp_set(&config, "C++20");
    project_file_dependency_add(&config, "zlib");
    project_file_dependency_add(&config, "m");
    list_add(&config.build.defines, strdup("FOO=1"));
    list_add(&config.build.include_dirs, strdup("third_party/x"));
    list_add(&config.build.cflags, strdup("-Wshadow"));
    list_add(&config.build.cxxflags, strdup("-fno-rtti"));
    list_add(&config.build.ldflags, strdup("-static"));
    list_add(&config.build.libs, strdup("dl"));
    CHECK(project_file_save(&config));
    project_file_config_clean(&config);

    char *text = idl_test_read(PROJECT_FILE_NAME);
    CHECK(text && strstr(text, "build:"));
    free(text);

    project_file_config_init(&config);
    CHECK(project_file_read(&config));
    CHECK_STR(config.name, "nome: \"estranho\"");
    CHECK_STR(config.version, "1.0");
    CHECK_STR(config.description, "linha1\nlinha2");
    CHECK_STR(config.requires_c, "C17");
    CHECK_STR(config.requires_cpp, "C++20");
    CHECK_INT(config.dependencies.count, 2);
    CHECK(project_file_dependency_find(&config, "zlib"));
    CHECK(project_file_dependency_find(&config, "m"));
    CHECK(idl_test_list_has(&config.build.defines, "FOO=1"));
    CHECK(idl_test_list_has(&config.build.include_dirs, "third_party/x"));
    CHECK(idl_test_list_has(&config.build.cflags, "-Wshadow"));
    CHECK(idl_test_list_has(&config.build.cxxflags, "-fno-rtti"));
    CHECK(idl_test_list_has(&config.build.ldflags, "-static"));
    CHECK(idl_test_list_has(&config.build.libs, "dl"));
    project_file_config_clean(&config);
}

static void test_dependencies(void) {
    project_config_t config = {0};
    project_file_config_init(&config);

    CHECK(project_file_dependency_add(&config, "zlib"));
    CHECK(project_file_dependency_add(&config, "zlib")); // sem duplicar
    CHECK(project_file_dependency_add(&config, "m"));
    CHECK_INT(config.dependencies.count, 2);

    CHECK(project_file_dependency_remove(&config, "zlib"));
    CHECK_INT(config.dependencies.count, 1);
    CHECK(!project_file_dependency_find(&config, "zlib"));
    CHECK(project_file_dependency_find(&config, "m"));

    project_file_config_clean(&config);
}

static void test_hand_written_file(void) {
    idl_test_enter_dir("hand");
    idl_test_write(PROJECT_FILE_NAME,
        "# comentário\n"
        "project:\n"
        "  name: manual\n"
        "  dependencies:\n"
        "build:\n"
        "  defines: [A=1, B]\n");

    project_config_t config = {0};
    project_file_config_init(&config);
    CHECK(project_file_read(&config));
    CHECK_STR(config.name, "manual");
    CHECK(config.version == nullptr);
    CHECK_INT(config.dependencies.count, 0);
    CHECK_INT(config.build.defines.count, 2);
    CHECK(idl_test_list_has(&config.build.defines, "B"));
    project_file_config_clean(&config);
}

static void check_invalid(const char *name, const char *content) {
    idl_test_enter_dir(name);
    idl_test_write(PROJECT_FILE_NAME, content);

    project_config_t config = {0};
    project_file_config_init(&config);
    CHECK(!project_file_read(&config));
    project_file_config_clean(&config);
}

static void test_invalid_files(void) {
    check_invalid("syntax", "project: {\n");
    check_invalid("no_project", "outro:\n  name: x\n");
    check_invalid("project_list", "project: [1, 2]\n");
    check_invalid("name_list", "project:\n  name: [a]\n");
    check_invalid("deps_scalar", "project:\n  dependencies: zlib\n");
    check_invalid("deps_nested", "project:\n  dependencies:\n  - [a]\n");
    check_invalid("build_scalar", "project:\n  name: x\nbuild: 1\n");
    check_invalid("cflags_scalar", "project:\n  name: x\nbuild:\n  cflags: -O3\n");
}

int main(void) {
    RUN_TEST(test_missing_file);
    RUN_TEST(test_save_and_read_basic);
    RUN_TEST(test_full_roundtrip);
    RUN_TEST(test_dependencies);
    RUN_TEST(test_hand_written_file);
    RUN_TEST(test_invalid_files);
    return idl_test_report();
}
