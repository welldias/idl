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

    // Without requires-cpp or build lists, those keys are not written to the file.
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
    project_file_name_set(&config, "name: \"weird\"");
    project_file_version_set(&config, "1.0");
    project_file_description_set(&config, "line1\nline2");
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
    CHECK_STR(config.name, "name: \"weird\"");
    CHECK_STR(config.version, "1.0");
    CHECK_STR(config.description, "line1\nline2");
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
    CHECK(project_file_dependency_add(&config, "zlib")); // no duplicates
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
        "# comment\n"
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

static project_target_config_t *find_target(project_config_t *config, const char *name) {
    for (list_item_t *item = config->targets.head; item; item = item->next) {
        project_target_config_t *target = (project_target_config_t *)item->value;
        if (strcmp(target->name, name) == 0)
            return target;
    }
    return nullptr;
}

static void test_targets(void) {
    idl_test_enter_dir("targets");
    idl_test_write(PROJECT_FILE_NAME,
        "project:\n"
        "  name: app\n"
        "targets:\n"
        "  core:\n"
        "    type: static-library\n"
        "    sources: [lib/core/**/*.c, lib/common.c]\n"
        "    exclude: [lib/core/legacy/**]\n"
        "    include-dirs: [lib/core/private]\n"
        "    public-include-dirs: [lib/core/include]\n"
        "    defines: [CORE=1]\n"
        "  app:\n"
        "    type: executable\n"
        "    sources: [apps/*.c]\n"
        "    link: [core]\n"
        "    libs: [pthread]\n"
        "  both:\n"
        "    type: library\n"
        "    sources: [x.c]\n"
        "  dyn:\n"
        "    type: shared-library\n"
        "    sources: [y.cpp]\n"
        "    cxxflags: [-fno-rtti]\n");

    project_config_t config = {0};
    project_file_config_init(&config);
    CHECK(project_file_read(&config));
    CHECK_INT(config.targets.count, 4);

    // Kept in the order of the file.
    CHECK_STR(((project_target_config_t *)config.targets.head->value)->name, "core");

    project_target_config_t *core = find_target(&config, "core");
    CHECK(core && core->type == PROJECT_TARGET_STATIC_LIBRARY);
    CHECK(core && core->sources.count == 2 && idl_test_list_has(&core->sources, "lib/core/**/*.c"));
    CHECK(core && idl_test_list_has(&core->exclude, "lib/core/legacy/**"));
    CHECK(core && idl_test_list_has(&core->include_dirs, "lib/core/private"));
    CHECK(core && idl_test_list_has(&core->public_include_dirs, "lib/core/include"));
    CHECK(core && idl_test_list_has(&core->defines, "CORE=1"));

    project_target_config_t *app = find_target(&config, "app");
    CHECK(app && app->type == PROJECT_TARGET_EXECUTABLE);
    CHECK(app && idl_test_list_has(&app->link, "core"));
    CHECK(app && idl_test_list_has(&app->libs, "pthread"));

    CHECK(find_target(&config, "both") && find_target(&config, "both")->type == PROJECT_TARGET_LIBRARY);
    CHECK(find_target(&config, "dyn") && find_target(&config, "dyn")->type == PROJECT_TARGET_SHARED_LIBRARY);

    // Saving (e.g. after "idl add") keeps the targets.
    project_file_dependency_add(&config, "zlib");
    CHECK(project_file_save(&config));
    project_file_config_clean(&config);
    CHECK_INT(config.targets.count, 0);

    project_file_config_init(&config);
    CHECK(project_file_read(&config));
    CHECK_INT(config.targets.count, 4);
    CHECK(project_file_dependency_find(&config, "zlib"));
    core = find_target(&config, "core");
    CHECK(core && core->type == PROJECT_TARGET_STATIC_LIBRARY);
    CHECK(core && core->sources.count == 2);
    CHECK(core && idl_test_list_has(&core->public_include_dirs, "lib/core/include"));
    CHECK(core && core->link.count == 0);
    app = find_target(&config, "app");
    CHECK(app && idl_test_list_has(&app->link, "core"));
    CHECK(find_target(&config, "dyn") && idl_test_list_has(&find_target(&config, "dyn")->cxxflags, "-fno-rtti"));
    project_file_config_clean(&config);
}

static void test_same_name(void) {
    idl_test_enter_dir("same_name");
    idl_test_write(PROJECT_FILE_NAME,
        "project:\n"
        "  name: lua\n"
        "targets:\n"
        "  lua:\n"
        "    type: static-library\n"
        "    sources: [src/*.c]\n"
        "  lua:\n"
        "    type: executable\n"
        "    sources: [src/lua.c]\n"
        "    link: [lua]\n");

    project_config_t config = {0};
    project_file_config_init(&config);
    CHECK(project_file_read(&config));
    CHECK_INT(config.targets.count, 2);

    // Saved and read again: both are kept.
    CHECK(project_file_save(&config));
    project_file_config_clean(&config);
    project_file_config_init(&config);
    CHECK(project_file_read(&config));
    CHECK_INT(config.targets.count, 2);
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
    check_invalid("no_project", "other:\n  name: x\n");
    check_invalid("project_list", "project: [1, 2]\n");
    check_invalid("name_list", "project:\n  name: [a]\n");
    check_invalid("deps_scalar", "project:\n  dependencies: zlib\n");
    check_invalid("deps_nested", "project:\n  dependencies:\n  - [a]\n");
    check_invalid("build_scalar", "project:\n  name: x\nbuild: 1\n");
    check_invalid("cflags_scalar", "project:\n  name: x\nbuild:\n  cflags: -O3\n");

    check_invalid("targets_empty", "project:\n  name: x\ntargets:\n");
    check_invalid("targets_list", "project:\n  name: x\ntargets: [a, b]\n");
    check_invalid("target_scalar", "project:\n  name: x\ntargets:\n  a: 1\n");
    check_invalid("target_no_type", "project:\n  name: x\ntargets:\n  a:\n    sources: [a.c]\n");
    check_invalid("target_bad_type", "project:\n  name: x\ntargets:\n  a:\n    type: program\n    sources: [a.c]\n");
    check_invalid("target_no_sources", "project:\n  name: x\ntargets:\n  a:\n    type: executable\n");
    check_invalid("target_sources_scalar", "project:\n  name: x\ntargets:\n  a:\n    type: executable\n    sources: a.c\n");
    check_invalid("target_unknown_key", "project:\n  name: x\ntargets:\n  a:\n    type: executable\n    source: [a.c]\n");
    check_invalid("target_twice", "project:\n  name: x\ntargets:\n  a:\n    type: executable\n    sources: [a.c]\n  a:\n    type: executable\n    sources: [b.c]\n");
    check_invalid("two_libraries", "project:\n  name: x\ntargets:\n  a:\n    type: static-library\n    sources: [a.c]\n  a:\n    type: shared-library\n    sources: [b.c]\n");
    check_invalid("target_bad_name", "project:\n  name: x\ntargets:\n  a/b:\n    type: executable\n    sources: [a.c]\n");
}

int main(void) {
    RUN_TEST(test_missing_file);
    RUN_TEST(test_save_and_read_basic);
    RUN_TEST(test_full_roundtrip);
    RUN_TEST(test_dependencies);
    RUN_TEST(test_hand_written_file);
    RUN_TEST(test_targets);
    RUN_TEST(test_same_name);
    RUN_TEST(test_invalid_files);
    return idl_test_report();
}
