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

void project_file_config_init(project_config_t *config) {
    RETURN_IF_FAIL(config);
    memset(config, 0, sizeof(project_config_t));

    list_init(&config->dependencies, free);
    for (word i = 0; i < SIZE_OF_ARRAY(project_file_build_lists); i++)
        list_init(project_file_build_list(config, i), free);
}

void project_file_config_clean(project_config_t *config) {
    RETURN_IF_FAIL(config);

    list_clear(&config->dependencies);
    for (word i = 0; i < SIZE_OF_ARRAY(project_file_build_lists); i++)
        list_clear(project_file_build_list(config, i));
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
        log_error("%s:%zu: '%s' deve ser um valor simples.", PROJECT_FILE_NAME, node->start_mark.line + 1, key);
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

/* Lê uma lista de strings. Chave ausente ou vazia não é erro. */
static bool project_file_yaml_list_get(yaml_document_t *doc, yaml_node_t *mapping, const char *key, list_t *list) {
    yaml_node_t *node = project_file_yaml_get(doc, mapping, key);
    if (!node || (node->type == YAML_SCALAR_NODE && node->data.scalar.length == 0))
        return true;

    if (node->type != YAML_SEQUENCE_NODE) {
        log_error("%s:%zu: '%s' deve ser uma lista.", PROJECT_FILE_NAME, node->start_mark.line + 1, key);
        return false;
    }

    for (yaml_node_item_t *item = node->data.sequence.items.start; item < node->data.sequence.items.top; item++) {
        yaml_node_t *value = yaml_document_get_node(doc, *item);
        if (!value || value->type != YAML_SCALAR_NODE) {
            log_error("%s:%zu: os itens de '%s' devem ser valores simples.", PROJECT_FILE_NAME, node->start_mark.line + 1, key);
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
        log_error("Erro ao criar o documento YAML.");
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

    // A seção build só é gravada com as listas que tiverem itens.
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

    if (!built) {
        log_error("Erro ao montar o documento YAML.");
        yaml_document_delete(&doc);
        return false;
    }

    FILE *f = fopen(PROJECT_FILE_NAME, "wb");
    if (!f) {
        log_error("Erro ao tentar salvar o arquivo %s: %s", PROJECT_FILE_NAME, strerror(errno));
        yaml_document_delete(&doc);
        return false;
    }

    yaml_emitter_t emitter;
    yaml_emitter_initialize(&emitter);
    yaml_emitter_set_output_file(&emitter, f);
    yaml_emitter_set_unicode(&emitter, 1);

    // yaml_emitter_dump libera o documento, com ou sem sucesso.
    bool result = yaml_emitter_open(&emitter) && yaml_emitter_dump(&emitter, &doc) && yaml_emitter_close(&emitter);
    if (!result)
        log_error("Erro ao gerar o YAML: %s", emitter.problem ? emitter.problem : "desconhecido");

    yaml_emitter_delete(&emitter);
    if (fclose(f) != 0)
        result = false;

    if (!result) {
        log_error("Erro ao tentar salvar o arquivo %s.", PROJECT_FILE_NAME);
        return false;
    }

    log_debug("Arquivo %s criado com sucesso!", PROJECT_FILE_NAME);
    return true;
}

bool project_file_read(project_config_t *config) {
    RETURN_VAL_IF_FAIL(config, false);

    FILE *f = fopen(PROJECT_FILE_NAME, "rb");
    if (!f) {
        log_error("Erro ao abrir o arquivo %s: %s", PROJECT_FILE_NAME, strerror(errno));
        return false;
    }

    yaml_parser_t parser;
    yaml_document_t doc;
    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, f);

    bool loaded = yaml_parser_load(&parser, &doc);
    if (!loaded)
        log_error("%s:%zu: %s", PROJECT_FILE_NAME, parser.problem_mark.line + 1, parser.problem ? parser.problem : "erro de sintaxe");

    yaml_parser_delete(&parser);
    fclose(f);

    if (!loaded)
        return false;

    bool result = false;

    yaml_node_t *project = project_file_yaml_get(&doc, yaml_document_get_root_node(&doc), "project");
    if (!project || project->type != YAML_MAPPING_NODE) {
        log_error("%s: seção 'project' não encontrada.", PROJECT_FILE_NAME);
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
        log_error("%s:%zu: 'build' deve ser uma seção com chaves.", PROJECT_FILE_NAME, build->start_mark.line + 1);
        goto cleanup;
    }

    for (word i = 0; build && i < SIZE_OF_ARRAY(project_file_build_lists); i++) {
        if (!project_file_yaml_list_get(&doc, build, project_file_build_lists[i].key, project_file_build_list(config, i)))
            goto cleanup;
    }

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