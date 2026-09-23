#include "idl_test.h"
#include "cmd_args.h"

static const char *const with_value[] = { "project", "p", nullptr };
static const char *const flags[] = { "verbose", nullptr };
static const cmd_args_spec_t spec = { .with_value = with_value, .flags = flags, .max_positionals = -1 };

static void test_values_flags_and_positionals(void) {
    char *argv[] = { "idl", "build", "lua", "--verbose", "luac", "--project", "my dir", "-p", "x", "-" };
    cmd_args_t args = {0};
    CHECK(cmd_args_parse(&args, 10, argv, 2, &spec));

    CHECK(cmd_args_has_flag(&args, "verbose"));
    CHECK(cmd_args_get_value(&args, "verbose") == nullptr);
    CHECK_STR(cmd_args_get_value(&args, "project"), "my dir");
    CHECK_STR(cmd_args_get_value(&args, "p"), "x");
    CHECK(!cmd_args_has_flag(&args, "does-not-exist"));

    // A word after a flag is never taken as its value.
    CHECK_INT(args.positionals.count, 3);
    CHECK_STR((const char *)args.positionals.head->value, "lua");
    CHECK_STR((const char *)args.positionals.head->next->value, "luac");
    CHECK_STR((const char *)args.positionals.head->next->next->value, "-");

    cmd_args_clear(&args);
    CHECK(args.head == nullptr);
    CHECK_INT(args.positionals.count, 0);
}

static void test_equals_form(void) {
    char *argv[] = { "idl", "build", "--project=a=b", "-p=c" };
    cmd_args_t args = {0};
    CHECK(cmd_args_parse(&args, 4, argv, 2, &spec));
    CHECK_STR(cmd_args_get_value(&args, "project"), "a=b");
    CHECK_STR(cmd_args_get_value(&args, "p"), "c");
    cmd_args_clear(&args);
}

static void test_start_index(void) {
    char *argv[] = { "idl", "--ignored", "run", "tool" };
    cmd_args_t args = {0};
    CHECK(cmd_args_parse(&args, 4, argv, 3, &spec));
    CHECK_INT(args.positionals.count, 1);
    CHECK_STR((const char *)args.positionals.head->value, "tool");
    cmd_args_clear(&args);
}

static void check_fails(int argc, char *argv[], const cmd_args_spec_t *with_spec) {
    cmd_args_t args = {0};
    CHECK(!cmd_args_parse(&args, argc, argv, 2, with_spec));
    cmd_args_clear(&args);
}

static void test_errors(void) {
    char *unknown[] = { "idl", "build", "--release" };
    check_fails(3, unknown, &spec);

    char *missing[] = { "idl", "build", "--project" };
    check_fails(3, missing, &spec);

    char *flag_with_value[] = { "idl", "build", "--verbose=yes" };
    check_fails(3, flag_with_value, &spec);

    static const cmd_args_spec_t one = { .with_value = with_value, .max_positionals = 1 };
    char *two[] = { "idl", "run", "a", "b" };
    check_fails(4, two, &one);

    static const cmd_args_spec_t none = { .with_value = with_value, .max_positionals = 0 };
    char *loose[] = { "idl", "clean", "x" };
    check_fails(3, loose, &none);
}

static void test_no_args(void) {
    char *argv[] = { "idl", "build" };
    cmd_args_t args = {0};
    CHECK(cmd_args_parse(&args, 2, argv, 2, &spec));
    CHECK(args.head == nullptr);
    CHECK_INT(args.positionals.count, 0);
    cmd_args_clear(&args);
}

int main(void) {
    RUN_TEST(test_values_flags_and_positionals);
    RUN_TEST(test_equals_form);
    RUN_TEST(test_start_index);
    RUN_TEST(test_errors);
    RUN_TEST(test_no_args);
    return idl_test_report();
}
