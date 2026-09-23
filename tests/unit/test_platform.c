#include "idl_test.h"

static void count_file(const char *path, void *arg) {
    (void)path;
    (*(int *)arg)++;
}

static void test_make_dirs(void) {
    idl_test_enter_dir("make_dirs");

    CHECK(platform_make_dirs("a/b/c"));
    CHECK(platform_dir_exists("a"));
    CHECK(platform_dir_exists("a/b/c"));
    CHECK(platform_make_dirs("a/b/c")); // já existe: não é erro

    idl_test_write("arquivo", "x");
    CHECK(!platform_make_dirs("arquivo")); // existe, mas não é diretório
}

/* Regressão: no Unix, platform_file_exists retornava true só para diretórios. */
static void test_exists(void) {
    idl_test_enter_dir("exists");
    platform_make_dirs("dir");
    idl_test_write("file.txt", "x");

    CHECK(platform_file_exists("file.txt"));
    CHECK(!platform_file_exists("dir"));
    CHECK(!platform_file_exists("nao-existe"));

    CHECK(platform_dir_exists("dir"));
    CHECK(!platform_dir_exists("file.txt"));
    CHECK(!platform_dir_exists("nao-existe"));
}

static void test_mtime(void) {
    idl_test_enter_dir("mtime");

    CHECK_INT(platform_file_mtime("nao-existe"), -1);

    idl_test_write("old.txt", "x");
    idl_test_write("new.txt", "x");

    uv_fs_t req;
    uv_fs_utime(nullptr, &req, "old.txt", 1000000.0, 1000000.0, nullptr);
    uv_fs_req_cleanup(&req);
    uv_fs_utime(nullptr, &req, "new.txt", 2000000.5, 2000000.5, nullptr);
    uv_fs_req_cleanup(&req);

    int64 old_mtime = platform_file_mtime("old.txt");
    int64 new_mtime = platform_file_mtime("new.txt");
    CHECK_INT(old_mtime, 1000000LL * 1000000000LL);
    CHECK(new_mtime > old_mtime);
}

static void test_remove_tree(void) {
    idl_test_enter_dir("remove_tree");
    idl_test_write("root/a.txt", "a");
    idl_test_write("root/sub/b.txt", "b");
    idl_test_write("root/sub/deep/c.txt", "c");
    idl_test_write("solo.txt", "s");

    CHECK(platform_remove_tree("root"));
    CHECK(!platform_dir_exists("root"));

    CHECK(platform_remove_tree("solo.txt"));
    CHECK(!platform_file_exists("solo.txt"));

    CHECK(platform_remove_tree("nao-existe"));
}

static void test_scan_directory(void) {
    idl_test_enter_dir("scan");
    idl_test_write("src/a.c", "");
    idl_test_write("src/b.h", "");
    idl_test_write("src/sub/c.c", "");

    int c_files = 0;
    CHECK(platform_scan_directory("src", ".c", count_file, &c_files));
    CHECK_INT(c_files, 2);

    int all_files = 0;
    CHECK(platform_scan_directory("src", nullptr, count_file, &all_files));
    CHECK_INT(all_files, 3);

    CHECK(!platform_scan_directory("nao-existe", nullptr, count_file, &all_files));
}

static void test_exec(void) {
    char *ok[] = { IDL_TEST_FAKE_TOOL, nullptr };
    CHECK_INT(platform_exec(ok), 0);

    char *fail[] = { IDL_TEST_FAKE_TOOL, "--exit", "3", nullptr };
    CHECK_INT(platform_exec(fail), 3);

    char *missing[] = { "programa-que-nao-existe-idl", nullptr };
    CHECK_INT(platform_exec(missing), -1);
}

static void test_misc(void) {
    CHECK(platform_num_cores() >= 1);
    CHECK(platform_file_is_binary(IDL_TEST_FAKE_TOOL));

    idl_test_enter_dir("misc");
    idl_test_write("texto.txt", "nao sou binario");
    CHECK(!platform_file_is_binary("texto.txt"));
}

int main(void) {
    RUN_TEST(test_make_dirs);
    RUN_TEST(test_exists);
    RUN_TEST(test_mtime);
    RUN_TEST(test_remove_tree);
    RUN_TEST(test_scan_directory);
    RUN_TEST(test_exec);
    RUN_TEST(test_misc);
    return idl_test_report();
}
