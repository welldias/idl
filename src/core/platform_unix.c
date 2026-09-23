#include "platform.h"

bool platform_file_exists(const char *full_path) {
    RETURN_VAL_IF_FAIL(full_path, false);

    struct stat st = { 0 };
    if (stat(full_path, &st) != 0) {
        return false;
    }

    return !S_ISDIR(st.st_mode);
}

bool platform_make_dirs(const char *path) {
    RETURN_VAL_IF_FAIL(path, false);

    char buffer[1024];
    int len = snprintf(buffer, sizeof(buffer), "%s", path);
    RETURN_VAL_IF_FAIL(len > 0 && len < (int)sizeof(buffer), false);

    // Create each intermediate directory; errors here are checked at the end.
    for (char *p = buffer + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(buffer, 0755);
            *p = '/';
        }
    }

    if (mkdir(buffer, 0755) != 0 && errno != EEXIST) {
        log_error("Could not create directory %s: %s", path, strerror(errno));
        return false;
    }

    struct stat st = { 0 };
    return stat(buffer, &st) == 0 && S_ISDIR(st.st_mode);
}

bool platform_file_is_binary(const char *full_path) {
    RETURN_VAL_IF_FAIL(full_path, false);

    struct stat st = { 0 };
    if (stat(full_path, &st) == 0) {
        return S_ISREG(st.st_mode) && (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH));
    }

    return false;
}

bool platform_scan_directory(const char *dir_path, const char *extension, platform_scan_directory_cb add_file_cb, void* arg) {
    RETURN_VAL_IF_FAIL(dir_path, false);
    RETURN_VAL_IF_FAIL(add_file_cb, false);

    DIR *dir = opendir(dir_path);
    if (!dir) {
        return false;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        bool is_dir = false;
        if (entry->d_type == DT_DIR) {
            is_dir = true;
        } else if (entry->d_type == DT_UNKNOWN) {
            struct stat st;
            if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                is_dir = true;
            }
        }

        if (is_dir) {
            platform_scan_directory(full_path, extension, add_file_cb, arg);
        } else if (extension == nullptr) {
            add_file_cb(full_path, arg);
        } else {
            word name_len = strlen(entry->d_name);
            word ext_len  = strlen(extension);
            if (name_len > ext_len && strcmp(entry->d_name + name_len - ext_len, extension) == 0) {
                add_file_cb(full_path, arg);
            }
        }
    }
    closedir(dir);

    return true;
}

bool platform_processes_wait_async(void *proc, int ms) {

    long ns                  = ms * 1000 * 1000;
    struct timespec duration = {
        .tv_sec  = ns / (1000 * 1000 * 1000),
        .tv_nsec = ns % (1000 * 1000 * 1000),
    };

    pid_t proc_pid = (pid_t)(intptr_t)proc;

    int wstatus = 0;
    pid_t pid   = waitpid(proc_pid, &wstatus, WNOHANG);
    if (pid < 0) {
        log_error("Could not wait on command (pid %d): %s", proc_pid, strerror(errno));
        return false;
    }

    if (pid == 0) {
        nanosleep(&duration, NULL);
        return 0;
    }

    if (WIFEXITED(wstatus)) {
        int exit_status = WEXITSTATUS(wstatus);
        if (exit_status != 0) {
            log_error("command exited with exit code %d", exit_status);
            return -1;
        }

        return 1;
    }

    if (WIFSIGNALED(wstatus)) {
        log_error("command process was terminated by signal %d", WTERMSIG(wstatus));
        return -1;
    }

    nanosleep(&duration, NULL);
    return 0;
}

uint32 platform_num_cores(void) {
    uv_cpu_info_t* info;
    int count;
    
    int r = uv_cpu_info(&info, &count);
    if (r < 0) {
        log_error("CPU info error: %s", uv_strerror(r));
        return 1;
    }

    uv_free_cpu_info(info, count);

    return count;
}