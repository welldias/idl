#include "platform.h"

bool platform_file_exists(const char *full_path) {
    RETURN_VAL_IF_FAIL(full_path, false);

    DWORD attrib = GetFileAttributes(full_path);
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool platform_make_dirs(const char *path) {
    RETURN_VAL_IF_FAIL(path, false);

    char buffer[1024];
    int len = snprintf(buffer, sizeof(buffer), "%s", path);
    RETURN_VAL_IF_FAIL(len > 0 && len < (int)sizeof(buffer), false);

    // Cria cada diretório intermediário; erros aqui (ex.: "C:") são checados no final.
    for (char *p = buffer + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            char sep = *p;
            *p = '\0';
            _mkdir(buffer);
            *p = sep;
        }
    }

    if (_mkdir(buffer) != 0 && errno != EEXIST) {
        log_error("Could not create directory %s: %s", path, strerror(errno));
        return false;
    }

    DWORD attrib = GetFileAttributes(buffer);
    return attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY);
}

bool platform_file_is_binary(const char *full_path) {
    RETURN_VAL_IF_FAIL(full_path, false);

    DWORD type  = 0;
    bool result = GetBinaryTypeA(full_path, &type);
    return result && (type == SCS_32BIT_BINARY || type == SCS_64BIT_BINARY);
}

bool platform_scan_directory(const char *dir_path, const char *extension, platform_scan_directory_cb add_file_cb, void* arg) {
    RETURN_VAL_IF_FAIL(dir_path, false);
    RETURN_VAL_IF_FAIL(add_file_cb, false);

    struct _finddata_t file_info;
    intptr_t handle;
    char search_path[1024];

    snprintf(search_path, sizeof(search_path), "%s\\*.*", dir_path);

    handle = _findfirst(search_path, &file_info);
    if (handle != -1) {
        do {
            if (strcmp(file_info.name, ".") == 0 || strcmp(file_info.name, "..") == 0) {
                continue;
            }

            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s\\%s", dir_path, file_info.name);

            if (file_info.attrib & _A_SUBDIR) {
                platform_scan_directory(full_path, extension, add_file_cb, arg);
            } else if (extension == nullptr) {
                add_file_cb(full_path, arg);
            } else {
                word name_len = strlen(file_info.name);
                word ext_len  = strlen(extension);
                if (name_len > ext_len && strcmp(file_info.name + name_len - ext_len, extension) == 0) {
                    add_file_cb(full_path, arg);
                }
            }
        } while (_findnext(handle, &file_info) == 0);
        _findclose(handle);
    }

    return true;
}

#define NOB_WIN32_ERR_MSG_SIZE (4 * 1024)

static const char *nob_win32_error_message(DWORD err) {
    static char win32_err_msg[NOB_WIN32_ERR_MSG_SIZE] = { 0 };
    DWORD err_msg_size                                = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, LANG_USER_DEFAULT, win32_err_msg, NOB_WIN32_ERR_MSG_SIZE, NULL);

    if (err_msg_size == 0) {
        if (GetLastError() != ERROR_MR_MID_NOT_FOUND) {
            if (sprintf(win32_err_msg, "Could not get error message for 0x%lX", err) > 0) {
                return (char *)&win32_err_msg;
            } else {
                return NULL;
            }
        } else {
            if (sprintf(win32_err_msg, "Invalid Windows Error code (0x%lX)", err) > 0) {
                return (char *)&win32_err_msg;
            } else {
                return NULL;
            }
        }
    }

    while (err_msg_size > 1 && isspace(win32_err_msg[err_msg_size - 1])) {
        win32_err_msg[--err_msg_size] = '\0';
    }

    return win32_err_msg;
}

bool platform_processes_wait_async(void *proc, int ms) {
    RETURN_VAL_IF_FAIL(proc, false);

    HANDLE proc_handle = (HANDLE)proc;

    DWORD result = WaitForSingleObject(proc, ms);
    RETURN_VAL_IF_FAIL(result == WAIT_TIMEOUT, true);

    if (result == WAIT_FAILED) {
        log_error("Could not wait on child process: %s", nob_win32_error_message(GetLastError()));
        return false;
    }

    result = 0;
    if (!GetExitCodeProcess(proc, &result)) {
        log_error("Could not get process exit code: %s", nob_win32_error_message(GetLastError()));
        return false;
    }

    if (result != 0) {
        log_error("Command exited with exit code %lu", result);
        return false;
    }

    CloseHandle(proc);

    return true;
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