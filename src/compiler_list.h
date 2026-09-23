#ifndef IDL_COMPILER_LIST_H
#define IDL_COMPILER_LIST_H

#include "idl.h"

typedef enum {
    COMPILER_GCC = 0,
    COMPILER_GXX,
    COMPILER_CLANG,
    COMPILER_CLANGXX
} compiler_type_t;

typedef struct compiler_info_t{
    char* full_path;
    char* name;
    compiler_type_t type;
    struct compiler_info_t *next;
} compiler_info_t;

typedef struct {
    compiler_info_t* head;
    word count;
} compiler_list_t;

bool compiler_list_find(compiler_list_t* list);
bool compiler_list_contains(compiler_list_t *list, const char *full_path);
bool compiler_list_add(compiler_list_t *list, const char* full_path, const char* name, compiler_type_t compiler);
void compiler_list_clear(compiler_list_t* list);
compiler_info_t* compiler_list_get(compiler_list_t* list, word index);
compiler_info_t* compiler_list_get_by_name(compiler_list_t* list, const char* name);
compiler_info_t* compiler_list_get_by_type(compiler_list_t* list, compiler_type_t type);

#endif // IDL_COMPILER_LIST_H