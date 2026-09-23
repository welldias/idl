#ifndef IDL_LIST_H
#define IDL_LIST_H

#include "commons.h"

typedef struct list_item_t {
    void *value;
    struct list_item_t *next;
} list_item_t;

typedef struct {
    word count;
    list_item_t *head;
    common_item_destroy_cb destroy_cb;
} list_t;

typedef bool (*list_compare_cb)(const void *value, const void *item_value);

list_t *list_create(common_item_destroy_cb destroy_cb);
bool list_init(list_t *list, common_item_destroy_cb destroy_cb);
bool list_clear(list_t *list);
void list_destroy(list_t *list);
word list_count(list_t *list);
bool list_add(list_t *list, void *value);
bool list_remove(list_t *list, const void *value, list_compare_cb compare_cb);
bool list_contains(list_t *list, const void *value, list_compare_cb compare_cb);

#endif /* IDL_LIST_H */