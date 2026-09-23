#include "cmd_args.h"

void cmd_args_parse(cmd_args_t *args, int argc, char *argv[], int start_index) {
    RETURN_IF_FAIL(args);
    RETURN_IF_FAIL(argv);
    RETURN_IF_FAIL(start_index > 0);

    args->head = nullptr;

    for (int i = start_index; i < argc; i++) {
        if (argv[i][0] == '-') {
            const char *key = argv[i];
            while (*key == '-') key++; // Skip the '-' or '--' prefix

            const char *value = nullptr;
            // If there is a next item and it is not another flag, it is the value.
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                value = argv[i + 1];
                i++; // Skip the item, it was consumed as the value
            }

            cmd_arg_opt_t *opt = (cmd_arg_opt_t *)malloc(sizeof(cmd_arg_opt_t));
            if (opt) {
                opt->key = strdup(key);
                opt->value = value ? strdup(value) : nullptr;
                
                // Insert at the head of the list
                opt->next = args->head;
                args->head = opt;
            }
        }
    }
}

const char* cmd_args_get_value(cmd_args_t *args, const char *key) {
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
}