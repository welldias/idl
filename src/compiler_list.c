#include "compiler_list.h"

struct {
    const char* name;
    compiler_type_t type;
} static compiler_list_target[] = {
    { "gcc", COMPILER_GCC },
    { "g++", COMPILER_GXX },
    { "clang", COMPILER_CLANG },
    { "clang++", COMPILER_CLANGXX }
};

static void compiler_list_item_clear(compiler_info_t *item);

bool compiler_list_find(compiler_list_t* list) {
    RETURN_VAL_IF_FAIL(list, false);

    const char* env_path = getenv("PATH");
    RETURN_VAL_IF_FAIL(env_path, false);

    list_t path_list  = { 0 };
    list_init(&path_list, free);

    bool result = strutils_str_to_list(env_path, strlen(env_path), ENV_PATH_SEPARATOR, &path_list);
    if (!result || path_list.count == 0) {
        list_clear(&path_list);
        return false;
    }

    compiler_list_clear(list);

    list->head = nullptr;
    list->count = 0;

    list_item_t *path_it =  path_list.head;
    while (path_it) {

        for(int i = 0; i < SIZE_OF_ARRAY(compiler_list_target); i++) {
            word path_len = strlen(path_it->value) + 1 + strlen(compiler_list_target[i].name) + strlen(EXE_EXTENSION) + 1;
            char* full_path = (char*)malloc(path_len);

            if (full_path) {
                snprintf(full_path, path_len, "%s%c%s%s", (const char *)path_it->value, FOLDER_SEPARATOR, compiler_list_target[i].name, EXE_EXTENSION);

                if (platform_file_is_binary(full_path)) {
                    compiler_list_add(list, 
                        full_path, 
                        compiler_list_target[i].name,
                        compiler_list_target[i].type);
                }

                free(full_path);
            }
        }

        path_it =  path_it->next;
    }

    list_clear(&path_list);
    return list->count > 0;
}

bool compiler_list_contains(compiler_list_t *list, const char *full_path) {
    RETURN_VAL_IF_FAIL(list, false);
    RETURN_VAL_IF_FAIL(full_path, false);

    for (compiler_info_t *item = list->head; item; item = item->next) {
        if (item->full_path && strcmp(item->full_path, full_path) == 0)
            return true;
    }
    return false;
}

bool compiler_list_add(compiler_list_t *list, const char* full_path, const char* name, compiler_type_t compiler) {
    RETURN_VAL_IF_FAIL(list, false);
    RETURN_VAL_IF_FAIL(name, false);
    RETURN_VAL_IF_FAIL(full_path, false);

    if (compiler_list_contains(list, full_path))
        return true; 

    compiler_info_t *item = (compiler_info_t *)malloc(sizeof(compiler_info_t));
    if (item == NULL)
        return false;

    memset(item, 0, sizeof(compiler_info_t));
    item->full_path = strdup(full_path);
    item->name = strdup(name);
    item->type = compiler;

    if (list->head == nullptr) {
        list->head = item;
    } else {
        compiler_info_t *tail = list->head;
        while (tail->next != nullptr) {
            tail = tail->next;
        }
        tail->next = item;
    }
    
    list->count++;
    return true;
}

void compiler_list_clear(compiler_list_t* list) {
    RETURN_IF_FAIL(list);
    
    compiler_list_item_clear(list->head);
    memset(list, 0, sizeof(compiler_list_t));
}

compiler_info_t* compiler_list_get(compiler_list_t* list, word index) {
    RETURN_VAL_IF_FAIL(list, nullptr);
    RETURN_VAL_IF_FAIL(index < list->count, nullptr);

    compiler_info_t *item = list->head;

    word i = 0;
    while(i < index && item != nullptr) {
        item = item->next;
        i++;
    }

    return item;
}

compiler_info_t* compiler_list_get_by_name(compiler_list_t* list, const char* name) {
    RETURN_VAL_IF_FAIL(list, nullptr);
    RETURN_VAL_IF_FAIL(name, nullptr);

    compiler_info_t *item = list->head;

    for(int i = 0; i < list->count; i++) {
        if (item->name && strcmp(item->name, name) == 0)
            return item;
        item = item->next;
    }

    return nullptr;
}

compiler_info_t* compiler_list_get_by_type(compiler_list_t* list, compiler_type_t type) {
    RETURN_VAL_IF_FAIL(list, nullptr);

    compiler_info_t *item = list->head;

    for(int i = 0; i < list->count; i++) {
        if (item->type == type)
            return item;
        item = item->next;
    }

    return nullptr;
}

static void compiler_list_item_clear(compiler_info_t *item) {

    if (item == nullptr)
        return;

    if (item->next != nullptr)
        compiler_list_item_clear(item->next);

    free(item->full_path);
    free(item->name);
    free(item);
}