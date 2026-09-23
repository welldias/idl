#include "idl_test.h"
#include "build_target.h"

static build_target_t *add(build_targets_t *targets, const char *name, build_target_type_t type, const char *links) {
    build_target_t *target = build_targets_add(targets, name, type);
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s", links ? links : "");
    for (char *link = strtok(buffer, " "); link; link = strtok(nullptr, " "))
        list_add(&target->link, strdup(link));
    return target;
}

static word find(build_targets_t *targets, const char *name) {
    for (word i = 0; i < targets->count; i++) {
        if (strcmp(targets->items[i].name, name) == 0)
            return i;
    }
    return (word)-1;
}

/* Names of the closure of <name>, separated by spaces. */
static void closure_names(build_targets_t *targets, const char *name, bool for_link, char *out, word size) {
    word ids[16];
    word count = build_target_closure(targets, find(targets, name), for_link, ids);
    out[0] = '\0';
    for (word i = 0; i < count; i++) {
        strncat(out, targets->items[ids[i]].name, size - strlen(out) - 1);
        if (i + 1 < count)
            strncat(out, " ", size - strlen(out) - 1);
    }
}

static void test_resolve(void) {
    build_targets_t targets = {0};
    add(&targets, "app", BUILD_TARGET_EXECUTABLE, "net core");
    add(&targets, "net", BUILD_TARGET_STATIC_LIBRARY, "core");
    add(&targets, "core", BUILD_TARGET_STATIC_LIBRARY, "base");
    add(&targets, "base", BUILD_TARGET_STATIC_LIBRARY, nullptr);
    add(&targets, "plugin", BUILD_TARGET_SHARED_LIBRARY, "core");
    add(&targets, "host", BUILD_TARGET_EXECUTABLE, "plugin");
    add(&targets, "alone", BUILD_TARGET_STATIC_LIBRARY, nullptr);
    CHECK(build_targets_resolve(&targets));

    CHECK_INT(targets.items[find(&targets, "base")].level, 0);
    CHECK_INT(targets.items[find(&targets, "core")].level, 1);
    CHECK_INT(targets.items[find(&targets, "net")].level, 2);
    CHECK_INT(targets.items[find(&targets, "app")].level, 3);
    CHECK_INT(targets.items[find(&targets, "host")].level, 3);

    // Dependents before their dependencies, each once.
    char names[256];
    closure_names(&targets, "app", true, names, sizeof(names));
    CHECK_STR(names, "net core base");

    // The static libraries inside a shared library are not linked again by its users.
    closure_names(&targets, "host", true, names, sizeof(names));
    CHECK_STR(names, "plugin");
    closure_names(&targets, "host", false, names, sizeof(names)); // but their public headers are visible
    CHECK_STR(names, "plugin core base");

    // Everything that goes into a shared library is position independent code.
    CHECK(targets.items[find(&targets, "plugin")].pic);
    CHECK(targets.items[find(&targets, "core")].pic);
    CHECK(targets.items[find(&targets, "base")].pic);
    CHECK(!targets.items[find(&targets, "net")].pic);
    CHECK(!targets.items[find(&targets, "alone")].pic);
    CHECK(!targets.items[find(&targets, "app")].pic);

    build_targets_clear(&targets);
}

static void test_shared_chain(void) {
    build_targets_t targets = {0};
    add(&targets, "app", BUILD_TARGET_EXECUTABLE, "s1");
    add(&targets, "s1", BUILD_TARGET_SHARED_LIBRARY, "st");
    add(&targets, "st", BUILD_TARGET_STATIC_LIBRARY, "s2");
    add(&targets, "s2", BUILD_TARGET_SHARED_LIBRARY, nullptr);
    CHECK(build_targets_resolve(&targets));

    // A shared library needed by another one is also given to the linker.
    char names[256];
    closure_names(&targets, "app", true, names, sizeof(names));
    CHECK_STR(names, "s1 s2");
    closure_names(&targets, "s1", true, names, sizeof(names));
    CHECK_STR(names, "st s2");
    build_targets_clear(&targets);
}

static void check_resolve_fails(build_targets_t *targets) {
    CHECK(!build_targets_resolve(targets));
    build_targets_clear(targets);
}

static void test_errors(void) {
    build_targets_t targets = {0};
    add(&targets, "app", BUILD_TARGET_EXECUTABLE, "missing");
    check_resolve_fails(&targets);

    add(&targets, "a", BUILD_TARGET_STATIC_LIBRARY, "a");
    check_resolve_fails(&targets);

    add(&targets, "tool", BUILD_TARGET_EXECUTABLE, nullptr);
    add(&targets, "app", BUILD_TARGET_EXECUTABLE, "tool");
    check_resolve_fails(&targets);

    add(&targets, "a", BUILD_TARGET_STATIC_LIBRARY, "b");
    add(&targets, "b", BUILD_TARGET_STATIC_LIBRARY, "c");
    add(&targets, "c", BUILD_TARGET_STATIC_LIBRARY, "a");
    check_resolve_fails(&targets);
}

static void test_same_name_as_library(void) {
    // Convention: src/bin/<project>.c in a library project links the library, not itself.
    build_targets_t targets = {0};
    add(&targets, "calc", BUILD_TARGET_LIBRARY, nullptr);
    add(&targets, "calc", BUILD_TARGET_EXECUTABLE, "calc");
    CHECK(build_targets_resolve(&targets));
    CHECK_INT(targets.items[1].dep_count, 1);
    CHECK_INT(targets.items[1].deps[0], 0);
    build_targets_clear(&targets);
}

int main(void) {
    RUN_TEST(test_resolve);
    RUN_TEST(test_shared_chain);
    RUN_TEST(test_errors);
    RUN_TEST(test_same_name_as_library);
    return idl_test_report();
}
