// This code is a modification of the work done by rxi. 
// Its original version can be found at: https://github.com/rxi/log.c

#ifndef IDL_LOG_H
#define IDL_LOG_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

typedef struct {
    va_list ap;
    const char *fmt;
    const char *file;
    struct tm *time;
    void *udata;
    int line;
    int level;
} log_event_t;

typedef void (*log_Lock_cb)(bool lock, void *udata);

enum { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL };

#define log_trace(...) log_log(LOG_TRACE, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define log_info(...) log_log(LOG_INFO, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define log_warn(...) log_log(LOG_WARN, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_log(LOG_FATAL, __FILE_NAME__, __LINE__, __VA_ARGS__)

const char *log_level_string(int level);
void log_set_lock(log_Lock_cb fn, void *udata);
void log_set_level(int level);
void log_set_quiet(bool enable);
void log_max_file_size_set(long value);
void log_max_backup_files_set(unsigned char value);
void log_file_name_set(const char* work_dir, const char* name);
void log_log(int level, const char *file, int line, const char *fmt, ...);

#endif //IDL_LOG_H