#include "idl_test.h"
#include "cmd_args.h"

static void test_parse_values_and_flags(void) {
    char *argv[] = { "idl", "build", "--release", "--project", "my dir", "-p", "x", "loose" };
    cmd_args_t args = {0};
    cmd_args_parse(&args, 8, argv, 2);

    CHECK(cmd_args_has_flag(&args, "release"));
    CHECK(cmd_args_get_value(&args, "release") == nullptr); // the next item is another option
    CHECK_STR(cmd_args_get_value(&args, "project"), "my dir");
    CHECK_STR(cmd_args_get_value(&args, "p"), "x");
    CHECK(!cmd_args_has_flag(&args, "loose"));
    CHECK(!cmd_args_has_flag(&args, "does-not-exist"));
    CHECK(cmd_args_get_value(&args, "does-not-exist") == nullptr);

    cmd_args_clear(&args);
    CHECK(args.head == nullptr);
}

static void test_start_index(void) {
    char *argv[] = { "idl", "--ignored", "run", "--bin", "tool" };
    cmd_args_t args = {0};
    cmd_args_parse(&args, 5, argv, 3);

    CHECK(!cmd_args_has_flag(&args, "ignored"));
    CHECK_STR(cmd_args_get_value(&args, "bin"), "tool");

    cmd_args_clear(&args);
}

/* Current behavior: an option followed by a loose item takes that item as its value. */
static void test_flag_consumes_next_item(void) {
    char *argv[] = { "idl", "test", "--release", "name" };
    cmd_args_t args = {0};
    cmd_args_parse(&args, 4, argv, 2);

    CHECK(cmd_args_has_flag(&args, "release"));
    CHECK_STR(cmd_args_get_value(&args, "release"), "name");

    cmd_args_clear(&args);
}

static void test_no_args(void) {
    char *argv[] = { "idl", "build" };
    cmd_args_t args = {0};
    cmd_args_parse(&args, 2, argv, 2);

    CHECK(args.head == nullptr);
    CHECK(!cmd_args_has_flag(&args, "release"));

    cmd_args_clear(&args);
}

int main(void) {
    RUN_TEST(test_parse_values_and_flags);
    RUN_TEST(test_start_index);
    RUN_TEST(test_flag_consumes_next_item);
    RUN_TEST(test_no_args);
    return idl_test_report();
}
