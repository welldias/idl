#include "idl_test.h"
#include "cmd_args.h"

static void test_parse_values_and_flags(void) {
    char *argv[] = { "idl", "build", "--release", "--project", "meu dir", "-p", "x", "solto" };
    cmd_args_t args = {0};
    cmd_args_parse(&args, 8, argv, 2);

    CHECK(cmd_args_has_flag(&args, "release"));
    CHECK(cmd_args_get_value(&args, "release") == nullptr); // o próximo item é outra opção
    CHECK_STR(cmd_args_get_value(&args, "project"), "meu dir");
    CHECK_STR(cmd_args_get_value(&args, "p"), "x");
    CHECK(!cmd_args_has_flag(&args, "solto"));
    CHECK(!cmd_args_has_flag(&args, "nao-existe"));
    CHECK(cmd_args_get_value(&args, "nao-existe") == nullptr);

    cmd_args_clear(&args);
    CHECK(args.head == nullptr);
}

static void test_start_index(void) {
    char *argv[] = { "idl", "--ignorado", "run", "--bin", "tool" };
    cmd_args_t args = {0};
    cmd_args_parse(&args, 5, argv, 3);

    CHECK(!cmd_args_has_flag(&args, "ignorado"));
    CHECK_STR(cmd_args_get_value(&args, "bin"), "tool");

    cmd_args_clear(&args);
}

/* Comportamento atual: uma opção seguida de um item solto recebe esse item como valor. */
static void test_flag_consumes_next_item(void) {
    char *argv[] = { "idl", "test", "--release", "nome" };
    cmd_args_t args = {0};
    cmd_args_parse(&args, 4, argv, 2);

    CHECK(cmd_args_has_flag(&args, "release"));
    CHECK_STR(cmd_args_get_value(&args, "release"), "nome");

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
