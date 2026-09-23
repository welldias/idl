#include "platform.h"

/* Platform functions implemented with libuv, identical on Unix and Windows. */

bool platform_dir_exists(const char *path) {
    RETURN_VAL_IF_FAIL(path, false);

    uv_fs_t req;
    int r = uv_fs_stat(nullptr, &req, path, nullptr);
    bool result = r == 0 && (req.statbuf.st_mode & S_IFMT) == S_IFDIR;
    uv_fs_req_cleanup(&req);
    return result;
}

int64 platform_file_mtime(const char *path) {
    RETURN_VAL_IF_FAIL(path, -1);

    uv_fs_t req;
    int r = uv_fs_stat(nullptr, &req, path, nullptr);
    int64 result = r == 0 ? (int64)req.statbuf.st_mtim.tv_sec * 1000000000 + req.statbuf.st_mtim.tv_nsec : -1;
    uv_fs_req_cleanup(&req);
    return result;
}

bool platform_remove_tree(const char *path) {
    RETURN_VAL_IF_FAIL(path, false);

    uv_fs_t req;
    int r = uv_fs_lstat(nullptr, &req, path, nullptr);
    bool is_dir = r == 0 && (req.statbuf.st_mode & S_IFMT) == S_IFDIR;
    uv_fs_req_cleanup(&req);

    if (r == UV_ENOENT)
        return true;

    if (!is_dir) {
        r = uv_fs_unlink(nullptr, &req, path, nullptr);
        uv_fs_req_cleanup(&req);
        if (r < 0)
            log_error("Could not remove %s: %s", path, uv_strerror(r));
        return r == 0;
    }

    bool result = true;
    r = uv_fs_scandir(nullptr, &req, path, 0, nullptr);
    if (r >= 0) {
        uv_dirent_t entry;
        while (uv_fs_scandir_next(&req, &entry) != UV_EOF) {
            char child[1024];
            snprintf(child, sizeof(child), "%s/%s", path, entry.name);
            result = platform_remove_tree(child) && result;
        }
    }
    uv_fs_req_cleanup(&req);

    r = uv_fs_rmdir(nullptr, &req, path, nullptr);
    uv_fs_req_cleanup(&req);
    if (r < 0) {
        log_error("Could not remove %s: %s", path, uv_strerror(r));
        return false;
    }

    return result;
}

typedef struct {
    int64 exit_status;
} platform_exec_ctx_t;

static void platform_exec_on_exit(uv_process_t *process, int64_t exit_status, int term_signal) {
    platform_exec_ctx_t *ctx = (platform_exec_ctx_t *)process->data;
    ctx->exit_status = term_signal ? 128 + term_signal : exit_status;
    uv_close((uv_handle_t *)process, nullptr);
}

int platform_exec(char *const *argv) {
    RETURN_VAL_IF_FAIL(argv && argv[0], -1);

    uv_loop_t loop;
    int r = uv_loop_init(&loop);
    if (r) {
        log_error("Could not init event loop: %s", uv_strerror(r));
        return -1;
    }

    platform_exec_ctx_t ctx = { .exit_status = -1 };
    uv_process_t process = { .data = &ctx };

    uv_stdio_container_t child_stdio[3];
    for (int i = 0; i < 3; i++) {
        child_stdio[i].flags = UV_INHERIT_FD;
        child_stdio[i].data.fd = i;
    }

    uv_process_options_t options = {0};
    options.exit_cb = platform_exec_on_exit;
    options.file = argv[0];
    options.args = (char **)argv;
    options.stdio_count = 3;
    options.stdio = child_stdio;

    r = uv_spawn(&loop, &process, &options);
    if (r) {
        log_error("Could not run %s: %s", argv[0], uv_strerror(r));
        uv_close((uv_handle_t *)&process, nullptr);
    }

    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(&loop);

    return (int)ctx.exit_status;
}
