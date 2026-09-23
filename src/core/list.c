#include "list.h"
#include "strutils.h"

static void list_item_clear(list_item_t *item, common_item_destroy_cb destroy_cb);

list_t *list_create(common_item_destroy_cb destroy_cb) {
    auto list = (list_t *)malloc(sizeof(list_t));
    RETURN_VAL_IF_FAIL(list, nullptr);

    list->head       = nullptr;
    list->count      = 0;
    list->destroy_cb = destroy_cb;

    return list;
}

bool list_init(list_t *list, common_item_destroy_cb destroy_cb) {
    RETURN_VAL_IF_FAIL(list, false);

    list_clear(list);
    list->head       = nullptr;
    list->count      = 0;
    list->destroy_cb = destroy_cb;

    return true;
}

bool list_clear(list_t *list) {
    RETURN_VAL_IF_FAIL(list, false);

    list_item_clear(list->head, list->destroy_cb);
    list->head  = nullptr;
    list->count = 0;

    return true;
}

void list_destroy(list_t *list) {
    RETURN_IF_FAIL(list);

    list_clear(list);
    free(list);
}

word list_count(list_t *list) {
    RETURN_VAL_IF_FAIL(list, 0);

    return list->count;
}

bool list_add(list_t *list, void *value) {
    RETURN_VAL_IF_FAIL(list, false);
    RETURN_VAL_IF_FAIL(value, false);

    auto item = (list_item_t *)malloc(sizeof(list_item_t));
    if (item == NULL)
        return false;

    memset(item, 0, sizeof(list_item_t));
    item->value = value;

    if (list->head == nullptr) {
        list->head = item;
    } else {
        list_item_t *tail = list->head;
        while (tail->next != nullptr) {
            tail = tail->next;
        }
        tail->next = item;
    }

    list->count++;
    return true;
}

bool list_remove(list_t *list, const void *value, list_compare_cb compare_cb) {
    RETURN_VAL_IF_FAIL(list, false);
    RETURN_VAL_IF_FAIL(value, false);

    list_item_t *item = NULL;
    list_item_t *prev = NULL;

    for (item = list->head; item; item = item->next) {
        if (compare_cb(value, item->value)) {
            if (prev == NULL)
                list->head = item->next;
            else
                prev->next = item->next;

            if (list->destroy_cb)
                list->destroy_cb(item->value);
            free(item);

            list->count--;
            break;
        }
        prev = item;
    }
    return true;
}

bool list_contains(list_t *list, const void *value, list_compare_cb compare_cb) {
    RETURN_VAL_IF_FAIL(list, false);
    RETURN_VAL_IF_FAIL(compare_cb, false);

    for (list_item_t *item = list->head; item; item = item->next) {
        if (compare_cb(value, item->value))
            return true;
    }
    return false;
}

/*****************************************************************************/
/* Static Functions                                                          */
/*****************************************************************************/
static void list_item_clear(list_item_t *item, common_item_destroy_cb destroy_cb) {
    if (item == nullptr)
        return; // empty list

    if (item->next != nullptr)
        list_item_clear(item->next, destroy_cb);

    if (destroy_cb)
        destroy_cb(item->value);
    free(item);
}
