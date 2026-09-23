#include "idl_test.h"
#include "build_env.h"
#include "project_file.h"

static void test_valid_name(void) {
    CHECK(build_env_valid_name("PATH"));
    CHECK(build_env_valid_name("PKG_CONFIG_PATH"));
    CHECK(build_env_valid_name("_PRIVATE"));
    CHECK(build_env_valid_name("V2"));

    CHECK(!build_env_valid_name(""));
    CHECK(!build_env_valid_name("path"));
    CHECK(!build_env_valid_name("Path"));
    CHECK(!build_env_valid_name("2V"));
    CHECK(!build_env_valid_name("MY-VAR"));
    CHECK(!build_env_valid_name("MY VAR"));
}

static void check_expand(const char *value, const char *expected) {
    char *result = build_env_expand(value);
    CHECK_STR(result, expected);
    free(result);
}

static void test_expand(void) {
    idl_test_setenv("IDL_TEST_A", "alpha");
    idl_test_setenv("IDL_TEST_UNSET", nullptr);

    check_expand("", "");
    check_expand("plain", "plain");
    check_expand("${IDL_TEST_A}", "alpha");
    check_expand("/opt/bin:${IDL_TEST_A}:${IDL_TEST_A}", "/opt/bin:alpha:alpha");
    check_expand("[${IDL_TEST_UNSET}]", "[]");
    check_expand("cost $$5", "cost $5");
    check_expand("$HOME and $", "$HOME and $"); // only ${...} is expanded
    check_expand("$${IDL_TEST_A}", "${IDL_TEST_A}");

    char *result = build_env_expand("broken ${IDL_TEST_A");
    CHECK(result == nullptr);
    free(result);
}

static void add_env(list_t *envs, const char *name, const char *value) {
    project_env_t *env = (project_env_t *)calloc(1, sizeof(project_env_t));
    env->name = strdup(name);
    env->value = strdup(value);
    list_add(envs, env);
}

static void free_env(void *item) {
    project_env_t *env = (project_env_t *)item;
    free(env->name);
    free(env->value);
    free(env);
}

static void test_apply(void) {
    idl_test_setenv("IDL_TEST_OLD", "old");
    idl_test_setenv("IDL_TEST_NEW", nullptr);
    idl_test_setenv("idl_test_lower", nullptr);

    list_t envs = {0};
    list_init(&envs, free_env);
    add_env(&envs, "IDL_TEST_NEW", "new");
    add_env(&envs, "IDL_TEST_OLD", "${IDL_TEST_OLD}+${IDL_TEST_NEW}"); // sees the one defined before
    add_env(&envs, "idl_test_lower", "ignored");
    char *applied = nullptr;
    CHECK(build_env_apply(&envs, &applied));
    CHECK_STR(applied, "IDL_TEST_NEW=new\nIDL_TEST_OLD=${IDL_TEST_OLD}+${IDL_TEST_NEW}\n"); // as written
    free(applied);

    CHECK_STR(getenv("IDL_TEST_NEW"), "new");
    CHECK_STR(getenv("IDL_TEST_OLD"), "old+new");
    CHECK(getenv("idl_test_lower") == nullptr);
    list_clear(&envs);

    list_init(&envs, free_env);
    add_env(&envs, "IDL_TEST_BAD", "${OPEN");
    CHECK(!build_env_apply(&envs, &applied));
    free(applied);
    list_clear(&envs);

    list_init(&envs, free_env);
    CHECK(build_env_apply(&envs, &applied));
    CHECK_STR(applied, "");
    free(applied);
    list_clear(&envs);
}

int main(void) {
    RUN_TEST(test_valid_name);
    RUN_TEST(test_expand);
    RUN_TEST(test_apply);
    return idl_test_report();
}
