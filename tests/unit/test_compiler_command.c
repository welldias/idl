#include "idl_test.h"
#include "compiler_command.h"

/* Regressão: o NULL final era gravado em args[count] em vez de args[cmd->count]. */
static void test_append_keeps_null_terminator(void) {
    compiler_command_t cmd = {0};
    compiler_command_init(&cmd, 2);

    COMPILER_COMMANDS_APPEND(&cmd, "gcc", "-c", "a.c");
    CHECK_INT(cmd.count, 3);
    CHECK(cmd.args[3] == nullptr);

    COMPILER_COMMANDS_APPEND(&cmd, "-o", "a.o");
    CHECK_INT(cmd.count, 5);
    CHECK(cmd.args[5] == nullptr);

    CHECK_STR(cmd.args[0], "gcc");
    CHECK_STR(cmd.args[2], "a.c");
    CHECK_STR(cmd.args[4], "a.o");

    compiler_command_clear(&cmd);
    CHECK(cmd.args == nullptr);
    CHECK_INT(cmd.count, 0);
}

static void test_many_appends(void) {
    compiler_command_t cmd = {0};
    compiler_command_init(&cmd, 1);

    for (int i = 0; i < 100; i++)
        compiler_commands_append(&cmd, 1, (char *[]){ "-Wall" });

    CHECK_INT(cmd.count, 100);
    CHECK(cmd.args[100] == nullptr);
    CHECK_STR(cmd.args[99], "-Wall");

    compiler_command_clear(&cmd);
}

int main(void) {
    RUN_TEST(test_append_keeps_null_terminator);
    RUN_TEST(test_many_appends);
    return idl_test_report();
}
