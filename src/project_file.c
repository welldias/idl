#include "project_file.h"
#include <yaml.h>

static bool project_file_compare_cb(const void *value, const void *item_value) {
    char *dependency = (char *)value;
    char *item_dependency = (char *)item_value;

    return strutils_cmp(dependency, strlen(dependency), item_dependency, strlen(item_dependency)) == 0;
}

static const struct {
    const char *key;
    size_t offset;
} project_file_build_lists[] = {
    { "defines", offsetof(project_build_config_t, defines) },
    { "include-dirs", offsetof(project_build_config_t, include_dirs) },
    { "cflags", offsetof(project_build_config_t, cflags) },
    { "cxxflags", offsetof(project_build_config_t, cxxflags) },
    { "ldflags", offsetof(project_build_config_t, ldflags) },
    { "libs", offsetof(project_build_config_t, libs) },
};

static list_t *project_file_build_list(project_config_t *config, word index) {
    return (list_t *)((byte *)&config->build + project_file_build_lists[index].offset);
}

static const struct {
    const char *key;
    size_t offset;
} project_file_target_lists[] = {
    { "sources", offsetof(project_target_config_t, sources) },
    { "exclude", offsetof(project_target_config_t, exclude) },
    { "include-dirs", offsetof(project_target_config_t, include_dirs) },
    { "public-include-dirs", offsetof(project_target_config_t, public_include_dirs) },
    { "defines", offsetof(project_target_config_t, defines) },
    { "cflags", offsetof(project_target_config_t, cflags) },
    { "cxxflags", offsetof(project_target_config_t, cxxflags) },
    { "ldflags", offsetof(project_target_config_t, ldflags) },
    { "libs", offsetof(project_target_config_t, libs) },
    { "link", offsetof(project_target_config_t, link) },
};

static list_t *project_file_target_list(project_target_config_t *target, word index) {
    return (list_t *)((byte *)target + project_file_target_lists[index].offset);
}

static const char *project_target_type_names[] = {
    [PROJECT_TARGET_EXECUTABLE] = "executable",
    [PROJECT_TARGET_STATIC_LIBRARY] = "static-library",
    [PROJECT_TARGET_SHARED_LIBRARY] = "shared-library",
    [PROJECT_TARGET_LIBRARY] = "library",
};

const char *project_target_type_name(project_target_type_t type) {
    return (word)type < SIZE_OF_ARRAY(project_target_type_names) ? project_target_type_names[type] : "?";
}

static project_target_config_t *project_target_create(const char *name, word name_len) {
    project_target_config_t *target = (project_target_config_t *)calloc(1, sizeof(project_target_config_t));
    if (!target) {
        LOG_FATAL_NOT_ENOUGH_MEMORY();
        return nullptr;
    }

    target->name = strutils_strndup(name, (int)name_len);
    for (word i = 0; i < SIZE_OF_ARRAY(project_file_target_lists); i++)
        list_init(project_file_target_list(target, i), free);
    return target;
}

static void project_target_destroy(void *item) {
    project_target_config_t *target = (project_target_config_t *)item;
    if (!target)
        return;

    for (word i = 0; i < SIZE_OF_ARRAY(project_file_target_lists); i++)
        list_clear(project_file_target_list(target, i));
    free(target->name);
    free(target);
}

void project_file_config_init(project_config_t *config) {
    RETURN_IF_FAIL(config);
    memset(config, 0, sizeof(project_config_t));

    list_init(&config->dependencies, free);
    for (word i = 0; i < SIZE_OF_ARRAY(project_file_build_lists); i++)
        list_init(project_file_build_list(config, i), free);
    list_init(&config->targets, project_target_destroy);
}

void project_file_config_clean(project_config_t *config) {
    RETURN_IF_FAIL(config);

    list_clear(&config->dependencies);
    for (word i = 0; i < SIZE_OF_ARRAY(project_file_build_lists); i++)
        list_clear(project_file_build_list(config, i));
    list_clear(&config->targets);
    free(config->name);
    free(config->version);
    free(config->description);
    free(config->requires_c);
    free(config->requires_cpp);
    project_file_config_init(config);
}

static int project_file_yaml_scalar_add(yaml_document_t *doc, const char *value) {
    return yaml_document_add_scalar(doc, NULL, (yaml_char_t *)(value ? value : ""), -1, YAML_ANY_SCALAR_STYLE);
}

static bool project_file_yaml_pair_add(yaml_document_t *doc, int mapping, const char *key, int value) {
    int key_id = project_file_yaml_scalar_add(doc, key);
    return key_id && value && yaml_document_append_mapping_pair(doc, mapping, key_id, value);
}

static yaml_node_t *project_file_yaml_get(yaml_document_t *doc, yaml_node_t *mapping, const char *key) {
    if (!mapping || mapping->type != YAML_MAPPING_NODE)
        return NULL;

    for (yaml_node_pair_t *pair = mapping->data.mapping.pairs.start; pair < mapping->data.mapping.pairs.top; pair++) {
        yaml_node_t *key_node = yaml_document_get_node(doc, pair->key);
        if (key_node && key_node->type == YAML_SCALAR_NODE && strcmp((const char *)key_node->data.scalar.value, key) == 0)
            return yaml_document_get_node(doc, pair->value);
    }
    return NULL;
}

static bool project_file_yaml_string_get(yaml_document_t *doc, yaml_node_t *mapping, const char *key, char **dest) {
    yaml_node_t *node = project_file_yaml_get(doc, mapping, key);
    if (!node)
        return true;

    if (node->type != YAML_SCALAR_NODE) {
        log_error("%s:%zu: '%s' must be a scalar value.", PROJECT_FILE_NAME, node->start_mark.line + 1, key);
        return false;
    }

    free(*dest);
    *dest = strutils_strndup((const char *)node->data.scalar.value, node->data.scalar.length);
    return true;
}

static int project_file_yaml_sequence_add(yaml_document_t *doc, list_t *list) {
    int sequence = yaml_document_add_sequence(doc, NULL, YAML_BLOCK_SEQUENCE_STYLE);
    for (list_item_t *item = list->head; sequence && item; item = item->next) {
        int value = project_file_yaml_scalar_add(doc, (const char *)item->value);
        if (!value || !yaml_document_append_sequence_item(doc, sequence, value))
            return 0;
    }
    return sequence;
}

/* Reads a list of strings. A missing or empty key is not an error. */
static bool project_file_yaml_list_get(yaml_document_t *doc, yaml_node_t *mapping, const char *key, list_t *list) {
    yaml_node_t *node = project_file_yaml_get(doc, mapping, key);
    if (!node || (node->type == YAML_SCALAR_NODE && node->data.scalar.length == 0))
        return true;

    if (node->type != YAML_SEQUENCE_NODE) {
        log_error("%s:%zu: '%s' must be a list.", PROJECT_FILE_NAME, node->start_mark.line + 1, key);
        return false;
    }

    for (yaml_node_item_t *item = node->data.sequence.items.start; item < node->data.sequence.items.top; item++) {
        yaml_node_t *value = yaml_document_get_node(doc, *item);
        if (!value || value->type != YAML_SCALAR_NODE) {
            log_error("%s:%zu: the items of '%s' must be scalar values.", PROJECT_FILE_NAME, node->start_mark.line + 1, key);
            return false;
        }

        const char *str = (const char *)value->data.scalar.value;
        if (!list_contains(list, str, project_file_compare_cb))
            list_add(list, strutils_strndup(str, value->data.scalar.length));
    }
    return true;
}

bool project_file_save(project_config_t *config) {
    RETURN_VAL_IF_FAIL(config, false);

    yaml_document_t doc;
    if (!yaml_document_initialize(&doc, NULL, NULL, NULL, 1, 1)) {
        log_error("Failed to create the YAML document.");
        return false;
    }

    int root = yaml_document_add_mapping(&doc, NULL, YAML_BLOCK_MAPPING_STYLE);
    int project = yaml_document_add_mapping(&doc, NULL, YAML_BLOCK_MAPPING_STYLE);

    bool built = root && project &&
                 project_file_yaml_pair_add(&doc, root, "project", project) &&
                 project_file_yaml_pair_add(&doc, project, "name", project_file_yaml_scalar_add(&doc, config->name)) &&
                 project_file_yaml_pair_add(&doc, project, "version", project_file_yaml_scalar_add(&doc, config->version)) &&
                 project_file_yaml_pair_add(&doc, project, "description", project_file_yaml_scalar_add(&doc, config->description)) &&
                 project_file_yaml_pair_add(&doc, project, "requires-c", project_file_yaml_scalar_add(&doc, config->requires_c));

    if (built && config->requires_cpp)
        built = project_file_yaml_pair_add(&doc, project, "requires-cpp", project_file_yaml_scalar_add(&doc, config->requires_cpp));

    if (built)
        built = project_file_yaml_pair_add(&doc, project, "dependencies", project_file_yaml_sequence_add(&doc, &config->dependencies));

    // The build section is only written with the lists that have items.
    int build = 0;
    for (word i = 0; built && i < SIZE_OF_ARRAY(project_file_build_lists); i++) {
        list_t *list = project_file_build_list(config, i);
        if (list->count == 0)
            continue;

        if (!build) {
            build = yaml_document_add_mapping(&doc, NULL, YAML_BLOCK_MAPPING_STYLE);
            built = build && project_file_yaml_pair_add(&doc, root, "build", build);
        }
        built = built && project_file_yaml_pair_add(&doc, build, project_file_build_lists[i].key, project_file_yaml_sequence_add(&doc, list));
    }

    int targets = 0;
    for (list_item_t *item = config->targets.head; built && item; item = item->next) {
        project_target_config_t *target = (project_target_config_t *)item->value;

        if (!targets) {
            targets = yaml_document_add_mapping(&doc, NULL, YAML_BLOCK_MAPPING_STYLE);
            built = targets && project_file_yaml_pair_add(&doc, root, "targets", targets);
        }

        int mapping = yaml_document_add_mapping(&doc, NULL, YAML_BLOCK_MAPPING_STYLE);
        built = built && mapping && project_file_yaml_pair_add(&doc, targets, target->name, mapping) &&
                project_file_yaml_pair_add(&doc, mapping, "type", project_file_yaml_scalar_add(&doc, project_target_type_name(target->type)));

        for (word i = 0; built && i < SIZE_OF_ARRAY(project_file_target_lists); i++) {
            list_t *list = project_file_target_list(target, i);
            if (list->count)
                built = project_file_yaml_pair_add(&doc, mapping, project_file_target_lists[i].key, project_file_yaml_sequence_add(&doc, list));
        }
    }

    if (!built) {
        log_error("Failed to build the YAML document.");
        yaml_document_delete(&doc);
        return false;
    }

    FILE *f = fopen(PROJECT_FILE_NAME, "wb");
    if (!f) {
        log_error("Failed to save %s: %s", PROJECT_FILE_NAME, strerror(errno));
        yaml_document_delete(&doc);
        return false;
    }

    yaml_emitter_t emitter;
    yaml_emitter_initialize(&emitter);
    yaml_emitter_set_output_file(&emitter, f);
    yaml_emitter_set_unicode(&emitter, 1);

    // yaml_emitter_dump frees the document, whether it succeeds or not.
    bool result = yaml_emitter_open(&emitter) && yaml_emitter_dump(&emitter, &doc) && yaml_emitter_close(&emitter);
    if (!result)
        log_error("Failed to write YAML: %s", emitter.problem ? emitter.problem : "unknown error");

    yaml_emitter_delete(&emitter);
    if (fclose(f) != 0)
        result = false;

    if (!result) {
        log_error("Failed to save %s.", PROJECT_FILE_NAME);
        return false;
    }

    log_debug("File %s saved successfully.", PROJECT_FILE_NAME);
    return true;
}

static bool project_file_target_read(yaml_document_t *doc, yaml_node_t *key, yaml_node_t *node, project_config_t *config) {
    const char *name = (const char *)key->data.scalar.value;
    word line = key->start_mark.line + 1;

    if (key->data.scalar.length == 0 || strpbrk(name, "/\\ \t")) {
        log_error("%s:%zu: invalid target name '%s' (it becomes a file name: no spaces or slashes).", PROJECT_FILE_NAME, line, name);
        return false;
    }

    if (!node || node->type != YAML_MAPPING_NODE) {
        log_error("%s:%zu: target '%s' must be a section with keys (type, sources...).", PROJECT_FILE_NAME, line, name);
        return false;
    }

    // Unknown keys are usually typos ("source:" instead of "sources:").
    for (yaml_node_pair_t *pair = node->data.mapping.pairs.start; pair < node->data.mapping.pairs.top; pair++) {
        yaml_node_t *pair_key = yaml_document_get_node(doc, pair->key);
        const char *key_name = pair_key && pair_key->type == YAML_SCALAR_NODE ? (const char *)pair_key->data.scalar.value : "";

        bool known = strcmp(key_name, "type") == 0;
        for (word i = 0; !known && i < SIZE_OF_ARRAY(project_file_target_lists); i++)
            known = strcmp(key_name, project_file_target_lists[i].key) == 0;
        if (!known) {
            log_error("%s:%zu: unknown key '%s' in target '%s'.", PROJECT_FILE_NAME, pair_key ? pair_key->start_mark.line + 1 : line, key_name, name);
            return false;
        }
    }

    project_target_config_t *target = project_target_create(name, key->data.scalar.length);
    if (!target || !list_add(&config->targets, target)) {
        project_target_destroy(target);
        return false;
    }

    char *type = nullptr;
    if (!project_file_yaml_string_get(doc, node, "type", &type))
        return false;

    bool type_found = false;
    for (word i = 0; type && i < SIZE_OF_ARRAY(project_target_type_names); i++) {
        if (strcmp(type, project_target_type_names[i]) == 0) {
            target->type = (project_target_type_t)i;
            type_found = true;
        }
    }
    if (!type_found) {
        if (type)
            log_error("%s:%zu: target '%s' has an unknown type '%s' (use executable, static-library, shared-library or library).", PROJECT_FILE_NAME, line, name, type);
        else
            log_error("%s:%zu: target '%s' has no type (executable, static-library, shared-library or library).", PROJECT_FILE_NAME, line, name);
        free(type);
        return false;
    }
    free(type);

    // An executable and a library can share a name (lua and liblua.a), two of the same kind cannot.
    bool executable = target->type == PROJECT_TARGET_EXECUTABLE;
    for (list_item_t *item = config->targets.head; item && item->value != target; item = item->next) {
        project_target_config_t *other = (project_target_config_t *)item->value;
        if (strcmp(other->name, name) == 0 && (other->type == PROJECT_TARGET_EXECUTABLE) == executable) {
            log_error("%s:%zu: %s '%s' is declared twice (an executable and a library may share a name, two %s may not).",
                      PROJECT_FILE_NAME, line, executable ? "executable" : "library", name, executable ? "executables" : "libraries");
            return false;
        }
    }

    for (word i = 0; i < SIZE_OF_ARRAY(project_file_target_lists); i++) {
        if (!project_file_yaml_list_get(doc, node, project_file_target_lists[i].key, project_file_target_list(target, i)))
            return false;
    }

    if (target->sources.count == 0) {
        log_error("%s:%zu: target '%s' has no sources.", PROJECT_FILE_NAME, line, name);
        return false;
    }
    return true;
}

static bool project_file_targets_read(yaml_document_t *doc, project_config_t *config) {
    yaml_node_t *targets = project_file_yaml_get(doc, yaml_document_get_root_node(doc), "targets");
    if (!targets)
        return true;

    if (targets->type != YAML_MAPPING_NODE || targets->data.mapping.pairs.start == targets->data.mapping.pairs.top) {
        log_error("%s:%zu: 'targets' must be a section with at least one target.", PROJECT_FILE_NAME, targets->start_mark.line + 1);
        return false;
    }

    for (yaml_node_pair_t *pair = targets->data.mapping.pairs.start; pair < targets->data.mapping.pairs.top; pair++) {
        yaml_node_t *key = yaml_document_get_node(doc, pair->key);
        if (!key || key->type != YAML_SCALAR_NODE) {
            log_error("%s:%zu: target names must be scalar values.", PROJECT_FILE_NAME, targets->start_mark.line + 1);
            return false;
        }
        if (!project_file_target_read(doc, key, yaml_document_get_node(doc, pair->value), config))
            return false;
    }
    return true;
}

bool project_file_read(project_config_t *config) {
    RETURN_VAL_IF_FAIL(config, false);

    FILE *f = fopen(PROJECT_FILE_NAME, "rb");
    if (!f) {
        log_error("Failed to open %s: %s", PROJECT_FILE_NAME, strerror(errno));
        return false;
    }

    yaml_parser_t parser;
    yaml_document_t doc;
    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, f);

    bool loaded = yaml_parser_load(&parser, &doc);
    if (!loaded)
        log_error("%s:%zu: %s", PROJECT_FILE_NAME, parser.problem_mark.line + 1, parser.problem ? parser.problem : "syntax error");

    yaml_parser_delete(&parser);
    fclose(f);

    if (!loaded)
        return false;

    bool result = false;

    yaml_node_t *project = project_file_yaml_get(&doc, yaml_document_get_root_node(&doc), "project");
    if (!project || project->type != YAML_MAPPING_NODE) {
        log_error("%s: section 'project' not found.", PROJECT_FILE_NAME);
        goto cleanup;
    }

    if (!project_file_yaml_string_get(&doc, project, "name", &config->name) ||
        !project_file_yaml_string_get(&doc, project, "version", &config->version) ||
        !project_file_yaml_string_get(&doc, project, "description", &config->description) ||
        !project_file_yaml_string_get(&doc, project, "requires-c", &config->requires_c) ||
        !project_file_yaml_string_get(&doc, project, "requires-cpp", &config->requires_cpp) ||
        !project_file_yaml_list_get(&doc, project, "dependencies", &config->dependencies))
        goto cleanup;

    yaml_node_t *build = project_file_yaml_get(&doc, yaml_document_get_root_node(&doc), "build");
    if (build && build->type != YAML_MAPPING_NODE) {
        log_error("%s:%zu: 'build' must be a section with keys.", PROJECT_FILE_NAME, build->start_mark.line + 1);
        goto cleanup;
    }

    for (word i = 0; build && i < SIZE_OF_ARRAY(project_file_build_lists); i++) {
        if (!project_file_yaml_list_get(&doc, build, project_file_build_lists[i].key, project_file_build_list(config, i)))
            goto cleanup;
    }

    if (!project_file_targets_read(&doc, config))
        goto cleanup;

    result = true;

cleanup:
    yaml_document_delete(&doc);
    return result;
}

bool project_file_name_set(project_config_t *config, const char *value) {
    RETURN_VAL_IF_FAIL(config, false);
    RETURN_VAL_IF_FAIL(value, false);

    free(config->name);
    config->name = strutils_strndup(value, strlen(value));

    return true;
}

bool project_file_version_set(project_config_t *config, const char *value) {
    RETURN_VAL_IF_FAIL(config, false);
    RETURN_VAL_IF_FAIL(value, false);

    free(config->version);
    config->version = strutils_strndup(value, strlen(value));

    return true;
}

bool project_file_description_set(project_config_t *config, const char *value) {
    RETURN_VAL_IF_FAIL(config, false);
    RETURN_VAL_IF_FAIL(value, false);

    free(config->description);
    config->description = strutils_strndup(value, strlen(value));

    return true;
}

bool project_file_requires_c_set(project_config_t *config, const char *value) {
    RETURN_VAL_IF_FAIL(config, false);
    RETURN_VAL_IF_FAIL(value, false);

    free(config->requires_c);
    config->requires_c = strutils_strndup(value, strlen(value));

    return true;
}

bool project_file_requires_cpp_set(project_config_t *config, const char *value) {
    RETURN_VAL_IF_FAIL(config, false);
    RETURN_VAL_IF_FAIL(value, false);

    free(config->requires_cpp);
    config->requires_cpp = strutils_strndup(value, strlen(value));

    return true;
}

bool project_file_dependency_add(project_config_t *config, const char *value) {
    RETURN_VAL_IF_FAIL(config, false);
    RETURN_VAL_IF_FAIL(value, false);

    if (!project_file_dependency_find(config, value))
        return list_add(&config->dependencies, strutils_strndup(value, strlen(value)));

    return true;
}

bool project_file_dependency_find(project_config_t *config, const char *value) {
    RETURN_VAL_IF_FAIL(config, false);
    RETURN_VAL_IF_FAIL(value, false);

    return list_contains(&config->dependencies, value, project_file_compare_cb);
}

bool project_file_dependency_remove(project_config_t *config, const char *value) {
    RETURN_VAL_IF_FAIL(config, false);
    RETURN_VAL_IF_FAIL(value, false);

    return list_remove(&config->dependencies, value, project_file_compare_cb);
}

bool project_file_exist() {
    FILE *f = fopen(PROJECT_FILE_NAME, "r");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}