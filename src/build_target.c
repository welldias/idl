#include "build_target.h"
#include "build_glob.h"

/* include_dirs ... link are consecutive list_t fields. */
static list_t *build_target_lists(build_target_t *target, word *count) {
    *count = (offsetof(build_target_t, link) - offsetof(build_target_t, include_dirs)) / sizeof(list_t) + 1;
    return &target->include_dirs;
}

build_target_t *build_targets_add(build_targets_t *targets, const char *name, build_target_type_t type) {
    RETURN_VAL_IF_FAIL(targets, nullptr);
    RETURN_VAL_IF_FAIL(name, nullptr);

    if (targets->count == targets->capacity) {
        word capacity = targets->capacity ? targets->capacity * 2 : 8;
        build_target_t *items = (build_target_t *)realloc(targets->items, capacity * sizeof(build_target_t));
        if (!items) {
            LOG_FATAL_NOT_ENOUGH_MEMORY();
            return nullptr;
        }
        targets->items = items;
        targets->capacity = capacity;
    }

    build_target_t *target = &targets->items[targets->count++];
    memset(target, 0, sizeof(build_target_t));
    target->name = strutils_strndup(name, (int)strlen(name));
    target->type = type;
    target->exe_kind = BUILD_ARTIFACT_EXE;
    target->obj_dir = strutils_format("");

    word list_count;
    list_t *lists = build_target_lists(target, &list_count);
    for (word i = 0; i < list_count; i++)
        list_init(&lists[i], free);
    return target;
}

void build_targets_clear(build_targets_t *targets) {
    RETURN_IF_FAIL(targets);

    for (word t = 0; t < targets->count; t++) {
        build_target_t *target = &targets->items[t];
        word list_count;
        list_t *lists = build_target_lists(target, &list_count);
        for (word i = 0; i < list_count; i++)
            list_clear(&lists[i]);
        build_sources_clear(&target->sources);
        free(target->name);
        free(target->obj_dir);
        free(target->deps);
    }
    free(targets->items);
    memset(targets, 0, sizeof(build_targets_t));
}

bool build_target_is_library(const build_target_t *target) {
    return target->type == BUILD_TARGET_STATIC_LIBRARY || target->type == BUILD_TARGET_SHARED_LIBRARY ||
           target->type == BUILD_TARGET_LIBRARY;
}

/*****************************************************************************/
/* Targets from project.yml                                                  */
/*****************************************************************************/

static void build_target_copy_list(list_t *dest, list_t *src) {
    for (list_item_t *item = src->head; item; item = item->next)
        list_add(dest, strutils_strndup((const char *)item->value, (int)strlen((const char *)item->value)));
}

/* A target with the settings of a "targets:" item. Its objects go to obj/<dir>/<item name>/. */
static build_target_t *build_targets_add_configured(build_targets_t *targets, project_target_config_t *source, const char *name,
                                                    build_target_type_t type, const char *dir) {
    build_target_t *target = build_targets_add(targets, name, type);
    if (!target)
        return nullptr;

    free(target->obj_dir);
    target->obj_dir = strutils_format("%s/%s/", dir, source->name);
    build_target_copy_list(&target->include_dirs, &source->include_dirs);
    build_target_copy_list(&target->public_include_dirs, &source->public_include_dirs);
    build_target_copy_list(&target->defines, &source->defines);
    build_target_copy_list(&target->cflags, &source->cflags);
    build_target_copy_list(&target->cxxflags, &source->cxxflags);
    build_target_copy_list(&target->ldflags, &source->ldflags);
    build_target_copy_list(&target->libs, &source->libs);
    build_target_copy_list(&target->link, &source->link);
    return target;
}

/* Tests end up in build/<profile>/tests/<name>: two with the same name would overwrite each other. */
static bool build_targets_test_name_free(build_targets_t *targets, const char *name, const char *source) {
    for (word i = 0; i < targets->count; i++) {
        build_target_t *other = &targets->items[i];
        if (other->type == BUILD_TARGET_EXECUTABLE && other->exe_kind == BUILD_ARTIFACT_TEST && strcmp(other->name, name) == 0) {
            log_error("Two tests would be named '%s' (%s and %s).", name, other->sources.count ? other->sources.items[0].path : other->name, source);
            return false;
        }
    }
    return true;
}

bool build_targets_from_config(build_targets_t *targets, project_config_t *config, bool with_tests) {
    RETURN_VAL_IF_FAIL(targets, false);
    RETURN_VAL_IF_FAIL(config, false);

    static const build_target_type_t types[] = {
        [PROJECT_TARGET_EXECUTABLE] = BUILD_TARGET_EXECUTABLE,
        [PROJECT_TARGET_STATIC_LIBRARY] = BUILD_TARGET_STATIC_LIBRARY,
        [PROJECT_TARGET_SHARED_LIBRARY] = BUILD_TARGET_SHARED_LIBRARY,
        [PROJECT_TARGET_LIBRARY] = BUILD_TARGET_LIBRARY,
        [PROJECT_TARGET_TEST] = BUILD_TARGET_EXECUTABLE,
    };

    for (list_item_t *item = config->targets.head; item; item = item->next) {
        project_target_config_t *source = (project_target_config_t *)item->value;
        bool test = source->type == PROJECT_TARGET_TEST;
        if (test && !with_tests)
            continue;

        build_sources_t sources = {0};
        char *owner = strutils_format("Target '%s'", source->name);
        bool expanded = build_glob_expand(&source->sources, &source->exclude, owner, &sources);
        free(owner);
        if (expanded && sources.count == 0)
            log_error("Target '%s' has no sources to compile on this system.", source->name);
        if (!expanded || sources.count == 0) {
            build_sources_clear(&sources);
            return false;
        }

        // By kind, since an executable and a library may have the same name.
        const char *dir = test ? "test" : types[source->type] == BUILD_TARGET_EXECUTABLE ? "exe" : "lib";

        if (test && !source->single) {
            // Each source is a test program of its own, named after the file (like tests/ in the convention).
            bool ok = true;
            for (word i = 0; ok && i < sources.count; i++) {
                build_source_t *file = &sources.items[i];
                if (!build_targets_test_name_free(targets, file->stem, file->path)) {
                    ok = false;
                    break;
                }

                build_target_t *target = build_targets_add_configured(targets, source, file->stem, BUILD_TARGET_EXECUTABLE, dir);
                if (target)
                    target->exe_kind = BUILD_ARTIFACT_TEST;
                ok = target && build_sources_add(&target->sources, file->path, file->lang);
            }
            build_sources_clear(&sources);
            if (!ok)
                return false;
            continue;
        }

        build_target_t *target = nullptr;
        if (!test || build_targets_test_name_free(targets, source->name, sources.items[0].path))
            target = build_targets_add_configured(targets, source, source->name, types[source->type], dir);
        if (!target) {
            build_sources_clear(&sources);
            return false;
        }
        if (test)
            target->exe_kind = BUILD_ARTIFACT_TEST;
        target->sources = sources; // the target takes ownership
    }
    return true;
}

/*****************************************************************************/
/* Links                                                                     */
/*****************************************************************************/

/* A link to a name shared by a library and an executable (lua and liblua.a in project.yml,
   or src/bin/<project>.c in a library project) goes to the library. */
static bool build_targets_find(build_targets_t *targets, word self, const char *name, word *found) {
    bool any = false;
    for (word i = 0; i < targets->count; i++) {
        if (i == self || strcmp(targets->items[i].name, name) != 0)
            continue;
        if (!any || targets->items[i].type != BUILD_TARGET_EXECUTABLE)
            *found = i;
        any = true;
    }
    return any;
}

/* Depth-first search: 0 not visited, 1 in the current path, 2 done. */
static bool build_targets_visit(build_targets_t *targets, word index, byte *state) {
    build_target_t *target = &targets->items[index];
    if (state[index] == 2)
        return true;
    if (state[index] == 1) {
        log_error("Target '%s' is part of a link cycle.", target->name);
        return false;
    }

    state[index] = 1;
    target->level = 0;
    for (word d = 0; d < target->dep_count; d++) {
        build_target_t *dep = &targets->items[target->deps[d]];
        if (!build_targets_visit(targets, target->deps[d], state))
            return false;
        if (dep->level + 1 > target->level)
            target->level = dep->level + 1;
    }
    state[index] = 2;
    return true;
}

bool build_targets_resolve(build_targets_t *targets) {
    RETURN_VAL_IF_FAIL(targets, false);

    for (word i = 0; i < targets->count; i++) {
        build_target_t *target = &targets->items[i];
        free(target->deps);
        target->dep_count = 0;
        target->deps = (word *)calloc(target->link.count ? target->link.count : 1, sizeof(word));
        if (!target->deps) {
            LOG_FATAL_NOT_ENOUGH_MEMORY();
            return false;
        }

        for (list_item_t *item = target->link.head; item; item = item->next) {
            const char *name = (const char *)item->value;
            word found = 0;
            if (!build_targets_find(targets, i, name, &found)) {
                if (strcmp(name, target->name) == 0)
                    log_error("Target '%s' links itself.", target->name);
                else
                    log_error("Target '%s' links '%s', which is not a target of the project.", target->name, name);
                return false;
            }
            if (targets->items[found].type == BUILD_TARGET_EXECUTABLE) {
                log_error("Target '%s' links '%s', which is an executable; only libraries can be linked.", target->name, name);
                return false;
            }
            target->deps[target->dep_count++] = found;
        }
    }

    byte *state = (byte *)calloc(targets->count ? targets->count : 1, 1);
    word *closure = (word *)calloc(targets->count ? targets->count : 1, sizeof(word));
    bool result = state && closure;
    if (!result)
        LOG_FATAL_NOT_ENOUGH_MEMORY();

    for (word i = 0; result && i < targets->count; i++)
        result = build_targets_visit(targets, i, state);

    // The objects that end up inside a shared library must be position independent.
    for (word i = 0; result && i < targets->count; i++) {
        build_target_t *target = &targets->items[i];
        if (target->type != BUILD_TARGET_SHARED_LIBRARY && target->type != BUILD_TARGET_LIBRARY)
            continue;

        target->pic = true;
        word count = build_target_closure(targets, i, true, closure);
        for (word c = 0; c < count; c++) {
            if (targets->items[closure[c]].type != BUILD_TARGET_SHARED_LIBRARY)
                targets->items[closure[c]].pic = true;
        }
    }

    free(state);
    free(closure);
    return result;
}

typedef struct {
    build_targets_t *targets;
    bool for_link;
    byte *visited;  // bit 0: visited outside a shared library, bit 1: inside
    bool *added;
    word *out;
    word count;
} build_closure_t;

static void build_closure_visit(build_closure_t *closure, word index, bool inside_shared) {
    build_target_t *target = &closure->targets->items[index];
    byte bit = inside_shared ? 2 : 1;
    if (closure->visited[index] & bit)
        return;
    closure->visited[index] |= bit;

    bool shared = target->type == BUILD_TARGET_SHARED_LIBRARY;
    bool inside = inside_shared || (closure->for_link && shared);
    for (word d = 0; d < target->dep_count; d++)
        build_closure_visit(closure, target->deps[d], inside);

    // Post-order: the dependencies of a target are written before it.
    if ((!inside_shared || shared) && !closure->added[index]) {
        closure->added[index] = true;
        closure->out[closure->count++] = index;
    }
}

word build_target_closure(build_targets_t *targets, word index, bool for_link, word *out) {
    RETURN_VAL_IF_FAIL(targets, 0);
    RETURN_VAL_IF_FAIL(out, 0);

    build_closure_t closure = {
        .targets = targets,
        .for_link = for_link,
        .visited = (byte *)calloc(targets->count, 1),
        .added = (bool *)calloc(targets->count, sizeof(bool)),
        .out = out,
    };
    if (!closure.visited || !closure.added) {
        LOG_FATAL_NOT_ENOUGH_MEMORY();
        free(closure.visited);
        free(closure.added);
        return 0;
    }

    closure.visited[index] = 3;
    closure.added[index] = true;
    build_target_t *target = &targets->items[index];
    for (word d = 0; d < target->dep_count; d++)
        build_closure_visit(&closure, target->deps[d], false);

    // Reversed post-order: dependents before their dependencies.
    for (word i = 0; i < closure.count / 2; i++) {
        word tmp = out[i];
        out[i] = out[closure.count - 1 - i];
        out[closure.count - 1 - i] = tmp;
    }

    free(closure.visited);
    free(closure.added);
    return closure.count;
}
