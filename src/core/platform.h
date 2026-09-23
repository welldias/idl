#ifndef IDL_PLATFORM_H
#define IDL_PLATFORM_H

#include "commons.h"
#include "list.h"
#include <uv.h>

typedef void (*platform_scan_directory_cb)(const char*, void* arg);

bool platform_file_exists(const char* full_path);
bool platform_file_is_binary(const char* full_path);
bool platform_make_dirs(const char* path);
bool platform_dir_exists(const char* path);
/* Returns the modification time in nanoseconds, or -1 if the file does not exist. */
int64 platform_file_mtime(const char* path);
/* Removes the file, or the directory with all its contents (like rm -rf). */
bool platform_remove_tree(const char* path);
/* Runs a program with inherited stdin/stdout/stderr and returns its exit code (-1 if it cannot start). */
int platform_exec(char* const* argv);
/* extension == NULL calls add_file_cb for every file. */
bool platform_scan_directory(const char *dir_path, const char *extension, platform_scan_directory_cb add_file_cb, void* arg);
bool platform_processes_wait_async(void * proc, int ms);
uint32 platform_num_cores(void);

#endif //IDL_PLATFORM_H