#include "cmd_args.h"

static bool cmd_args_in(const char *const *list, const char *key, word key_len) {
    for (; list && *list; list++) {
        if (strlen(*list) == key_len && strncmp(*list, key, key_len) == 0)
            return true;
    }
    return false;
}

static bool cmd_args_add(cmd_args_t *args, const char *key, word key_len, const char *value) {
    cmd_arg_opt_t *opt = (cmd_arg_opt_t *)calloc(1, sizeof(cmd_arg_opt_t));
    if (!opt) {
        LOG_FATAL_NOT_ENOUGH_MEMORY();
        return false;
    }

    opt->key = strutils_format("%.*s", (int)key_len, key);
    opt->value = value ? strutils_format("%s", value) : nullptr;
    opt->next = args->head;
    args->head = opt;
    return opt->key != nullptr;
}

bool cmd_args_parse(cmd_args_t *args, int argc, char *argv[], int start_index, const cmd_args_spec_t *spec) {
    RETURN_VAL_IF_FAIL(args, false);
    RETURN_VAL_IF_FAIL(argv, false);
    RETURN_VAL_IF_FAIL(spec, false);
    RETURN_VAL_IF_FAIL(start_index > 0, false);

    args->head = nullptr;
    list_init(&args->positionals, nullptr);

    for (int i = start_index; i < argc; i++) {
        const char *arg = argv[i];

        // "-" alone is an ordinary argument (e.g. stdin), like any word without a leading dash.
        if (arg[0] != '-' || arg[1] == '\0') {
            if (spec->max_positionals >= 0 && (int)args->positionals.count >= spec->max_positionals) {
                log_error("Unexpected argument '%s'.", arg);
                return false;
            }
            list_add(&args->positionals, (void *)arg);
            continue;
        }

        const char *key = arg;
        while (*key == '-')
            key++; // "-p" or "--project"
        const char *equals = strchr(key, '=');
        word key_len = equals ? (word)(equals - key) : strlen(key);

        if (cmd_args_in(spec->with_value, key, key_len)) {
            const char *value = equals ? equals + 1 : (i + 1 < argc ? argv[++i] : nullptr);
            if (!value) {
                log_error("Option '%s' needs a value.", arg);
                return false;
            }
            if (!cmd_args_add(args, key, key_len, value))
                return false;
        } else if (cmd_args_in(spec->flags, key, key_len) && !equals) {
            if (!cmd_args_add(args, key, key_len, nullptr))
                return false;
        } else {
            log_error("Unknown option '%s'.", arg);
            return false;
        }
    }
    return true;
}

const char *cmd_args_get_value(cmd_args_t *args, const char *key) {
    RETURN_VAL_IF_FAIL(args, nullptr);
    RETURN_VAL_IF_FAIL(key, nullptr);

    cmd_arg_opt_t *curr = args->head;
    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            return curr->value;
        }
        curr = curr->next;
    }
    return nullptr;
}

bool cmd_args_has_flag(cmd_args_t *args, const char *key) {
    RETURN_VAL_IF_FAIL(args, false);
    RETURN_VAL_IF_FAIL(key, false);

    cmd_arg_opt_t *curr = args->head;
    while (curr) {
        if (strcmp(curr->key, key) == 0) return true;
        curr = curr->next;
    }
    return false;
}

void cmd_args_clear(cmd_args_t *args) {
    RETURN_IF_FAIL(args);
    
    cmd_arg_opt_t *curr = args->head;
    while (curr) {
        cmd_arg_opt_t *next = curr->next;
        free(curr->key);
        if (curr->value) free(curr->value);
        free(curr);
        curr = next;
    }
    args->head = nullptr;
    list_clear(&args->positionals);
}