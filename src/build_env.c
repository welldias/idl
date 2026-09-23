#include "build_env.h"
#include "project_file.h"

#include <uv.h>

bool build_env_valid_name(const char *name) {
    RETURN_VAL_IF_FAIL(name, false);

    if (!((*name >= 'A' && *name <= 'Z') || *name == '_'))
        return false;
    for (const char *c = name + 1; *c; c++) {
        if (!((*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9') || *c == '_'))
            return false;
    }
    return true;
}

/* Value of an environment variable (release with free), or an empty string if it is not set. */
static char *build_env_get(const char *name) {
    size_t size = 256;
    char *value = (char *)malloc(size);
    if (!value)
        return nullptr;

    int r = uv_os_getenv(name, value, &size);
    if (r == UV_ENOBUFS) {
        char *bigger = (char *)realloc(value, size);
        if (!bigger) {
            free(value);
            return nullptr;
        }
        value = bigger;
        r = uv_os_getenv(name, value, &size);
    }
    if (r != 0)
        value[0] = '\0';
    return value;
}

char *build_env_expand(const char *value) {
    RETURN_VAL_IF_FAIL(value, nullptr);

    stream_t out = {0};
    stream_init(&out, strlen(value) + 1);

    for (const char *p = value; *p; p++) {
        if (p[0] == '$' && p[1] == '$') {
            stream_write(&out, (const byte *)"$", 1);
            p++;
            continue;
        }
        if (p[0] != '$' || p[1] != '{') {
            stream_write(&out, (const byte *)p, 1);
            continue;
        }

        const char *end = strchr(p + 2, '}');
        if (!end) {
            stream_clear(&out);
            return nullptr;
        }

        char *name = strutils_strndup(p + 2, (int)(end - p - 2));
        char *current = name ? build_env_get(name) : nullptr;
        if (current)
            stream_write(&out, (const byte *)current, strlen(current));
        free(current);
        free(name);
        p = end;
    }

    stream_write(&out, (const byte *)"", 1);
    return (char *)out.data; // the caller takes ownership of the buffer
}

bool build_env_apply(list_t *envs, char **applied) {
    RETURN_VAL_IF_FAIL(envs, false);
    RETURN_VAL_IF_FAIL(applied, false);

    stream_t out = {0};
    stream_init(&out, 64);
    bool result = true;

    for (list_item_t *item = envs->head; result && item; item = item->next) {
        project_env_t *env = (project_env_t *)item->value;
        if (!build_env_valid_name(env->name)) {
            log_warn("envs: '%s' ignored: environment variable names use uppercase letters, digits and _.", env->name);
            continue;
        }

        char *value = build_env_expand(env->value);
        if (!value) {
            log_error("envs: the value of %s has a '${' without the closing '}'.", env->name);
            result = false;
            break;
        }

        int r = uv_os_setenv(env->name, value);
        if (r != 0) {
            log_error("envs: could not set %s: %s", env->name, uv_strerror(r));
            result = false;
        } else {
            log_debug("envs: %s=%s", env->name, value);
            stream_write_string(&out, "%s=%s\n", env->name, env->value);
        }
        free(value);
    }

    stream_write(&out, (const byte *)"", 1);
    *applied = (char *)out.data; // the caller takes ownership of the buffer
    return result;
}
