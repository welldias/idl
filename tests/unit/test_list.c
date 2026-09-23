#include "idl_test.h"

static int destroyed = 0;

static void count_destroy(void *value) {
    destroyed++;
    free(value);
}

static bool str_equals(const void *value, const void *item_value) {
    return strcmp((const char *)value, (const char *)item_value) == 0;
}

static list_t make_list(common_item_destroy_cb destroy_cb, const char **values, int count) {
    list_t list = {0};
    list_init(&list, destroy_cb);
    for (int i = 0; i < count; i++)
        list_add(&list, strdup(values[i]));
    return list;
}

static void test_init_empty(void) {
    list_t list = {0};
    CHECK(list_init(&list, free));
    CHECK_INT(list_count(&list), 0);
    CHECK(list.head == nullptr);
    CHECK(list_clear(&list)); // an empty list is not an error
}

static void test_add_keeps_order(void) {
    const char *values[] = { "a", "b", "c" };
    list_t list = make_list(free, values, 3);

    CHECK_INT(list_count(&list), 3);
    CHECK_STR((const char *)list.head->value, "a");
    CHECK_STR((const char *)list.head->next->value, "b");
    CHECK_STR((const char *)list.head->next->next->value, "c");
    CHECK(!list_add(&list, nullptr));

    list_clear(&list);
}

static void test_contains(void) {
    const char *values[] = { "gcc", "clang" };
    list_t list = make_list(free, values, 2);

    CHECK(list_contains(&list, "clang", str_equals));
    CHECK(!list_contains(&list, "tcc", str_equals));

    list_clear(&list);
}

/* Regression: list_remove removed the first item that did NOT match. */
static void test_remove_matching_item(void) {
    const char *values[] = { "a", "b", "c" };
    list_t list = make_list(free, values, 3);

    list_remove(&list, "b", str_equals);
    CHECK_INT(list_count(&list), 2);
    CHECK(list_contains(&list, "a", str_equals));
    CHECK(!list_contains(&list, "b", str_equals));
    CHECK(list_contains(&list, "c", str_equals));

    list_remove(&list, "a", str_equals);
    CHECK_STR((const char *)list.head->value, "c");

    list_remove(&list, "does-not-exist", str_equals);
    CHECK_INT(list_count(&list), 1);

    list_clear(&list);
}

static void test_clear_calls_destroy(void) {
    const char *values[] = { "1", "2", "3" };
    destroyed = 0;
    list_t list = make_list(count_destroy, values, 3);

    list_clear(&list);
    CHECK_INT(destroyed, 3);
    CHECK_INT(list_count(&list), 0);
    CHECK(list.head == nullptr);
}

/* Regression: without destroy_cb, list_clear did not free the nodes. */
static void test_clear_without_destroy(void) {
    char a[] = "a", b[] = "b";
    list_t list = {0};
    list_init(&list, nullptr);
    list_add(&list, a);
    list_add(&list, b);

    list_clear(&list);
    CHECK_INT(list_count(&list), 0);
    CHECK_STR(a, "a"); // the values are not freed
}

static void test_create_destroy(void) {
    destroyed = 0;
    list_t *list = list_create(count_destroy);
    CHECK(list != nullptr);
    list_add(list, strdup("x"));
    list_destroy(list);
    CHECK_INT(destroyed, 1);
}

int main(void) {
    RUN_TEST(test_init_empty);
    RUN_TEST(test_add_keeps_order);
    RUN_TEST(test_contains);
    RUN_TEST(test_remove_matching_item);
    RUN_TEST(test_clear_calls_destroy);
    RUN_TEST(test_clear_without_destroy);
    RUN_TEST(test_create_destroy);
    return idl_test_report();
}
