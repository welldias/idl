#include "log.h"
#include "strutils.h"

#define LOG_PATH_FILE_NAME_LEN 512
#define LOG_BKP_FILE_NAME_PREFIX_LEN 5
#define LOG_1MB 1048576L

static struct {
    void *udata;
    log_Lock_cb lock;
    int level;
    bool quiet;

    FILE *output;
    char file_name[LOG_PATH_FILE_NAME_LEN + 1];
    long current_file_size;
    long max_file_size;
    unsigned char max_backup_files;
} L = {
    .max_file_size    = 5 * LOG_1MB, /* 5 MB */
    .max_backup_files = 5,
};

static const char *level_strings[] = { "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL" };

#ifdef LOG_USE_COLOR
static const char *level_colors[] = { "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m" };
#endif

static void lock(void) {
    if (L.lock) {
        L.lock(true, L.udata);
    }
}

static void unlock(void) {
    if (L.lock) {
        L.lock(false, L.udata);
    }
}

static void log_backup_file_name_get(const char *base_name, unsigned char index, char *bkp_name, long size) {

    strncpy(bkp_name, base_name, size);

    if (index > 0) {
        char index_name[LOG_BKP_FILE_NAME_PREFIX_LEN];

        sprintf(index_name, ".%d", index);
        strncat(bkp_name, index_name, LOG_BKP_FILE_NAME_PREFIX_LEN);
    }
}

static int log_is_file_exist(const char *name) {
    FILE *fp = NULL;

    if ((fp = fopen(name, "r")) == NULL) {
        return 0;
    }

    fclose(fp);
    return 1;
}

static long log_file_size_get(const char *name) {
    FILE *fp;
    long size;

    if ((fp = fopen(name, "rb")) == NULL) {
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fclose(fp);
    return size;
}

static int log_rotate_log_files(void) {

    if (L.output == NULL || (L.current_file_size < L.max_file_size)) {
        return 0;
    }

    int i;

    /* backup filename: <filename>.xxx (xxx: 1-255) */
    char src[LOG_PATH_FILE_NAME_LEN + LOG_BKP_FILE_NAME_PREFIX_LEN];
    char dst[LOG_PATH_FILE_NAME_LEN + LOG_BKP_FILE_NAME_PREFIX_LEN];

    fclose(L.output);

    for (i = (int)L.max_backup_files; i > 0; i--) {
        log_backup_file_name_get(L.file_name, i - 1, src, sizeof(src));
        log_backup_file_name_get(L.file_name, i, dst, sizeof(dst));

        if (log_is_file_exist(dst)) {
            if (remove(dst) != 0) {
                fprintf(stderr, "ERROR: logger: Failed to remove file: `%s`\n", dst);
            }
        }
        if (log_is_file_exist(src)) {
            if (rename(src, dst) != 0) {
                fprintf(stderr, "ERROR: logger: Failed to rename file: `%s` -> `%s`\n", src, dst);
            }
        }
    }

    L.output = fopen(L.file_name, "a");
    if (L.output == NULL) {
        fprintf(stderr, "ERROR: logger: Failed to open file: `%s`\n", L.file_name);
        return 0;
    }

    L.current_file_size = log_file_size_get(L.file_name);

    return 1;
}

static void stdout_log_function(log_event_t *ev) {
    char buf[16];
    buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';
#ifdef LOG_USE_COLOR
    fprintf(ev->udata, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ", buf, level_colors[ev->level], level_strings[ev->level], ev->file, ev->line);
#else
    fprintf(ev->udata, "%s %-5s %s:%d: ", buf, level_strings[ev->level], ev->file, ev->line);
#endif
    vfprintf(ev->udata, ev->fmt, ev->ap);
    fprintf(ev->udata, "\n");
    fflush(ev->udata);
}

static void file_log_function(log_event_t *ev) {

    if (!L.output)
        return;

    char buf[64];

    buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ev->time)] = '\0';
    L.current_file_size += fprintf(ev->udata, "%s %-5s %s:%d: ", buf, level_strings[ev->level], ev->file, ev->line);
    vfprintf(ev->udata, ev->fmt, ev->ap);
    L.current_file_size += fprintf(ev->udata, "\n");
    fflush(ev->udata);

    log_rotate_log_files();
}

const char *log_level_string(int level) {
    return level_strings[level];
}

void log_set_lock(log_Lock_cb fn, void *udata) {
    L.lock  = fn;
    L.udata = udata;
}

void log_set_level(int level) {
    L.level = level;
}

void log_set_quiet(bool enable) {
    L.quiet = enable;
}

void log_max_file_size_set(long value) {
    L.max_file_size = value * LOG_1MB;
}

void log_max_backup_files_set(unsigned char value) {
    L.max_backup_files = value;
}

void log_file_name_set(const char *work_dir, const char *name) {

    if (L.output) {
        fclose(L.output);
    }

    if (!name) {
        L.output       = NULL;
        L.file_name[0] = 0;
        return;
    }

    char format[20] = { 0 };

    int len = (work_dir == NULL ? 0 : strlen(work_dir));
    if (len > 0) {
        sprintf(format, "%%s%%s");
        if (work_dir[len - 1] != FOLDER_SEPARATOR) {
            sprintf(format, "%%s%c%%s", FOLDER_SEPARATOR);
            len += 1;
        }
    }

    len += strlen(name);

    if (len > LOG_PATH_FILE_NAME_LEN) {
        fprintf(stderr, "ERROR: log file name is too long. Maximum allowed size is %d.\n", LOG_PATH_FILE_NAME_LEN);
        return;
    }

    if (work_dir != NULL && strlen(work_dir) > 0) {
        snprintf(L.file_name, sizeof(L.file_name), format, work_dir, name);
    } else {
        snprintf(L.file_name, sizeof(L.file_name), "%s", name);
    }

    L.output = fopen(L.file_name, "a");
    if (L.output == NULL) {
        fprintf(stderr, "ERROR: Failed to open log file: `%s`\n", name);
        return;
    }

    L.current_file_size = log_file_size_get(L.file_name);
}

static void init_event(log_event_t *ev, void *udata) {
    if (!ev->time) {
        time_t t = time(NULL);
        ev->time = localtime(&t);
    }
    ev->udata = udata;
}

void log_log(int level, const char *file, int line, const char *fmt, ...) {
    log_event_t ev = {
        .fmt   = fmt,
        .file  = file,
        .line  = line,
        .level = level,
    };

    lock();

    if (level >= L.level) {
        if (!L.quiet) {
            init_event(&ev, stderr);
            va_start(ev.ap, fmt);
            stdout_log_function(&ev);
            va_end(ev.ap);
        }

        if (L.output) {
            init_event(&ev, L.output);
            va_start(ev.ap, fmt);
            file_log_function(&ev);
            va_end(ev.ap);
        }
    }

    unlock();
}
