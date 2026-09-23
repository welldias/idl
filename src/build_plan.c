#include "build_plan.h"
#include "build_env.h"
#include "compiler_command.h"
#include "process_runner.h"

#include <uv.h>

#define BUILD_COMPILE_COMMANDS BUILD_OUT_DIR "/compile_commands.json"

/* Shared library naming and flags for each platform. */
#if defined(_WIN32)
    #define BUILD_SHARED_PREFIX ""
    #define BUILD_SHARED_EXTENSION ".dll"
    #define BUILD_SHARED_NEEDS_PIC false
#elif defined(__APPLE__)
    #define BUILD_SHARED_PREFIX "lib"
    #define BUILD_SHARED_EXTENSION ".dylib"
    #define BUILD_SHARED_NEEDS_PIC true
#else
    #define BUILD_SHARED_PREFIX "lib"
    #define BUILD_SHARED_EXTENSION ".so"
    #define BUILD_SHARED_NEEDS_PIC true
#endif

struct build_unit_t {
    word target;
    build_source_t *source;
    char *obj;      // build/<profile>/obj/[<target>/]<source>.o
    char *dep;      // build/<profile>/obj/[<target>/]<source>.d   (written by the compiler with -MMD)
    char *stamp;    // build/<profile>/obj/[<target>/]<source>.cmd (command used in the last compilation)
    compiler_command_t cmd;
    bool dirty;
};

/* Keeps the string in the plan so it is freed along with it. */
static const char *build_own(build_plan_t *plan, char *str) {
    list_add(&plan->strings, str);
    return str;
}

static void build_arg(compiler_command_t *cmd, const char *arg) {
    compiler_commands_append(cmd, 1, (char *[]){ (char *)arg });
}

static void build_arg_list(build_plan_t *plan, compiler_command_t *cmd, list_t *list, const char *prefix) {
    for (list_item_t *item = list->head; item; item = item->next) {
        const char *value = (const char *)item->value;
        build_arg(cmd, prefix ? build_own(plan, strutils_format("%s%s", prefix, value)) : value);
    }
}

static char *build_read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return nullptr;

    char *content = nullptr;
    if (fseek(f, 0, SEEK_END) == 0) {
        long size = ftell(f);
        rewind(f);
        content = size >= 0 ? (char *)malloc(size + 1) : nullptr;
        if (content)
            content[fread(content, 1, size, f)] = '\0';
    }
    fclose(f);
    return content;
}

static bool build_make_parent_dirs(const char *path) {
    const char *slash = strrchr(path, '/');
    if (!slash)
        return true;

    char *dir = strutils_strndup(path, (int)(slash - path));
    bool result = platform_make_dirs(dir);
    free(dir);
    return result;
}

/*****************************************************************************/
/* Incremental build                                                         */
/*****************************************************************************/

static char *build_cmd_join(compiler_command_t *cmd) {
    word len = 1;
    for (word i = 0; i < cmd->count; i++)
        len += strlen(cmd->args[i]) + 1;

    char *joined = (char *)malloc(len);
    if (!joined)
        return nullptr;

    char *p = joined;
    for (word i = 0; i < cmd->count; i++)
        p += sprintf(p, "%s\n", cmd->args[i]);
    *p = '\0';
    return joined;
}

/* What produced an output: the command line plus the variables of envs:, which can also
   change the result (CPATH, PATH with another compiler...). */
static char *build_stamp_content(build_plan_t *plan, compiler_command_t *cmd) {
    char *joined = build_cmd_join(cmd);
    if (!joined || !plan->env_stamp || !plan->env_stamp[0])
        return joined;

    char *content = strutils_format("%s%s", joined, plan->env_stamp);
    free(joined);
    return content;
}

/* Did the command or the environment change since the last build? */
static bool build_stamp_matches(build_plan_t *plan, const char *stamp, compiler_command_t *cmd) {
    char *content = build_read_file(stamp);
    char *expected = build_stamp_content(plan, cmd);
    bool result = content && expected && strcmp(content, expected) == 0;
    free(content);
    free(expected);
    return result;
}

static void build_stamp_write(build_plan_t *plan, const char *stamp, compiler_command_t *cmd) {
    char *content = build_stamp_content(plan, cmd);
    FILE *f = content ? fopen(stamp, "wb") : nullptr;
    if (f) {
        fputs(content, f);
        fclose(f);
    }
    free(content);
}

/* Reads the first rule of the .d file ("obj: source header1 header2 ...") and
   checks whether any dependency is gone or newer than the object. */
static bool build_deps_changed(const char *dep_path, int64 obj_mtime) {
    char *content = build_read_file(dep_path);
    if (!content)
        return true;

    bool changed = false;
    bool in_deps = false;
    char token[1024];
    const char *p = content;

    while (*p && !changed) {
        if (*p == ' ' || *p == '\t' || *p == '\r') {
            p++;
            continue;
        }
        if (*p == '\\' && p[1] == '\n') {
            p += 2;
            continue;
        }
        if (*p == '\\' && p[1] == '\r' && p[2] == '\n') {
            p += 3;
            continue;
        }
        if (*p == '\n')
            break; // end of the first rule; the next ones are -MP phony targets

        const char *start = p;
        word len = 0;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') {
            if (*p == '\\' && (p[1] == '\n' || p[1] == '\r'))
                break;
            if ((*p == '\\' && p[1] == ' ') || (*p == '$' && p[1] == '$'))
                p++; // escaped space or "$$"
            if (len < sizeof(token) - 1)
                token[len++] = *p;
            p++;
        }
        token[len] = '\0';
        if (p == start)
            p++;

        if (!in_deps) {
            in_deps = len > 0 && token[len - 1] == ':';
            continue;
        }

        int64 mtime = platform_file_mtime(token);
        if (mtime < 0 || mtime > obj_mtime)
            changed = true;
    }

    free(content);
    return changed || !in_deps;
}

/*****************************************************************************/
/* Command lines                                                             */
/*****************************************************************************/

/* "C23" -> "-std=c23", "C++20" -> "-std=c++20", "gnu17" -> "-std=gnu17". */
static char *build_std_flag(const char *value, const char *key) {
    if (!value || !value[0])
        return nullptr;

    char *flag = strutils_format("-std=%s", value);
    for (char *p = flag + 5; *p; p++)
        *p = (char)tolower((unsigned char)*p);

    const char *std = flag + 5;
    if (std[0] != 'c' && strncmp(std, "gnu", 3) != 0) {
        log_warn("%s '%s' not recognized; no -std will be used.", key, value);
        free(flag);
        return nullptr;
    }
    return flag;
}

static void build_plan_compile_cmd(build_plan_t *plan, build_unit_t *unit, word *closure) {
    compiler_command_t *cmd = &unit->cmd;
    project_config_t *config = &plan->layout.config;
    build_targets_t *targets = &plan->layout.targets;
    build_target_t *target = &targets->items[unit->target];
    bool cxx = unit->source->lang == BUILD_LANG_CXX;

    compiler_command_init(cmd, 32);
    build_arg(cmd, cxx ? plan->toolchain.cxx : plan->toolchain.cc);

    char *std = cxx ? build_std_flag(config->requires_cpp, "requires-cpp")
                    : build_std_flag(config->requires_c, "requires-c");
    if (std)
        build_arg(cmd, build_own(plan, std));

    if (plan->options.release) {
        build_arg(cmd, "-O2");
        build_arg(cmd, "-DNDEBUG");
    } else {
        build_arg(cmd, "-g");
        build_arg(cmd, "-O0");
    }
    build_arg(cmd, "-Wall");
    build_arg(cmd, "-Wextra");
    if (target->pic && BUILD_SHARED_NEEDS_PIC)
        build_arg(cmd, "-fPIC");

    // Include path: the target's own directories, then the public ones of what it links, then the global ones.
    build_arg_list(plan, cmd, &target->include_dirs, "-I");
    build_arg_list(plan, cmd, &target->public_include_dirs, "-I");
    word count = build_target_closure(targets, unit->target, false, closure);
    for (word i = 0; i < count; i++)
        build_arg_list(plan, cmd, &targets->items[closure[i]].public_include_dirs, "-I");
    build_arg_list(plan, cmd, &config->build.include_dirs, "-I");

    // Global settings first, so the target's own can override them.
    build_arg_list(plan, cmd, &config->build.defines, "-D");
    build_arg_list(plan, cmd, &target->defines, "-D");
    build_arg_list(plan, cmd, &plan->toolchain.cflags, nullptr);
    build_arg_list(plan, cmd, cxx ? &config->build.cxxflags : &config->build.cflags, nullptr);
    build_arg_list(plan, cmd, cxx ? &target->cxxflags : &target->cflags, nullptr);

    build_arg(cmd, "-MMD");
    build_arg(cmd, "-MP");
    build_arg(cmd, "-MF");
    build_arg(cmd, unit->dep);
    build_arg(cmd, "-c");
    build_arg(cmd, unit->source->path);
    build_arg(cmd, "-o");
    build_arg(cmd, unit->obj);
}

static bool build_plan_create_units(build_plan_t *plan) {
    build_targets_t *targets = &plan->layout.targets;

    word total = 0;
    for (word t = 0; t < targets->count; t++)
        total += plan->wanted[t] ? targets->items[t].sources.count : 0;

    plan->units = (build_unit_t *)calloc(total ? total : 1, sizeof(build_unit_t));
    word *closure = (word *)calloc(targets->count ? targets->count : 1, sizeof(word));
    if (!plan->units || !closure) {
        LOG_FATAL_NOT_ENOUGH_MEMORY();
        free(closure);
        return false;
    }

    for (word t = 0; t < targets->count; t++) {
        build_target_t *target = &targets->items[t];
        target->first_unit = plan->unit_count;
        if (!plan->wanted[t])
            continue;

        for (word i = 0; i < target->sources.count; i++) {
            build_unit_t *unit = &plan->units[plan->unit_count++];
            unit->target = t;
            unit->source = &target->sources.items[i];
            unit->obj = strutils_format("%s/obj/%s%s.o", plan->out_dir, target->obj_dir, unit->source->path);
            unit->dep = strutils_format("%s/obj/%s%s.d", plan->out_dir, target->obj_dir, unit->source->path);
            unit->stamp = strutils_format("%s/obj/%s%s.cmd", plan->out_dir, target->obj_dir, unit->source->path);
            build_plan_compile_cmd(plan, unit, closure);
        }
    }

    free(closure);
    return true;
}

static char *build_plan_artifact_path(build_plan_t *plan, build_target_t *target, build_artifact_kind_t kind) {
    switch (kind) {
        case BUILD_ARTIFACT_LIB:
            return strutils_format("%s/lib%s.a", plan->out_dir, target->name);
        case BUILD_ARTIFACT_SHARED:
            return strutils_format("%s/%s%s%s", plan->out_dir, BUILD_SHARED_PREFIX, target->name, BUILD_SHARED_EXTENSION);
        case BUILD_ARTIFACT_TEST:
            return strutils_format("%s/tests/%s%s", plan->out_dir, target->name, EXE_EXTENSION);
        default:
            return strutils_format("%s/%s%s", plan->out_dir, target->name, EXE_EXTENSION);
    }
}

typedef struct {
    build_artifact_t artifact;
    word target;
} build_artifact_entry_t;

static void build_plan_add_artifact(build_plan_t *plan, build_artifact_entry_t *entries, word target, build_artifact_kind_t kind) {
    build_target_t *source = &plan->layout.targets.items[target];
    build_artifact_entry_t *entry = &entries[plan->artifact_count++];
    entry->artifact.kind = kind;
    entry->artifact.name = strutils_strndup(source->name, strlen(source->name));
    entry->artifact.path = build_plan_artifact_path(plan, source, kind);
    entry->target = target;
}

static bool build_plan_link(build_plan_t *plan, build_artifact_entry_t *entries, word *linked);

static bool build_plan_link_artifacts(build_plan_t *plan, word *linked) {
    build_targets_t *targets = &plan->layout.targets;

    build_artifact_entry_t *entries = (build_artifact_entry_t *)calloc(targets->count ? targets->count * 2 : 1, sizeof(build_artifact_entry_t));
    if (!entries) {
        LOG_FATAL_NOT_ENOUGH_MEMORY();
        return false;
    }

    for (word t = 0; t < targets->count; t++) {
        build_target_t *target = &targets->items[t];
        if (plan->wanted[t] == 0)
            continue;

        // A target that is only needed by another gives just what the linker uses from it.
        if (plan->wanted[t] == 1) {
            if (target->link_objects || target->type == BUILD_TARGET_OBJECTS)
                continue;
            build_plan_add_artifact(plan, entries, t, target->type == BUILD_TARGET_SHARED_LIBRARY ? BUILD_ARTIFACT_SHARED : BUILD_ARTIFACT_LIB);
            continue;
        }

        switch (target->type) {
            case BUILD_TARGET_OBJECTS:
                break;
            case BUILD_TARGET_EXECUTABLE:
                build_plan_add_artifact(plan, entries, t, target->exe_kind);
                break;
            case BUILD_TARGET_STATIC_LIBRARY:
                build_plan_add_artifact(plan, entries, t, BUILD_ARTIFACT_LIB);
                break;
            case BUILD_TARGET_SHARED_LIBRARY:
                build_plan_add_artifact(plan, entries, t, BUILD_ARTIFACT_SHARED);
                break;
            case BUILD_TARGET_LIBRARY:
                build_plan_add_artifact(plan, entries, t, BUILD_ARTIFACT_LIB);
                build_plan_add_artifact(plan, entries, t, BUILD_ARTIFACT_SHARED);
                break;
        }
    }

    bool result = build_plan_link(plan, entries, linked);

    plan->artifacts = (build_artifact_t *)calloc(plan->artifact_count ? plan->artifact_count : 1, sizeof(build_artifact_t));
    for (word i = 0; i < plan->artifact_count; i++) {
        if (plan->artifacts)
            plan->artifacts[i] = entries[i].artifact;
    }
    free(entries);

    return result && plan->artifacts;
}

static bool build_target_has_cxx(build_target_t *target) {
    for (word i = 0; i < target->sources.count; i++) {
        if (target->sources.items[i].lang == BUILD_LANG_CXX)
            return true;
    }
    return false;
}

/* Adds the objects of a target to the command and to the inputs of the artifact. */
static void build_plan_link_objects(build_plan_t *plan, build_target_t *target, compiler_command_t *cmd, list_t *inputs) {
    for (word i = 0; i < target->sources.count; i++) {
        const char *obj = plan->units[target->first_unit + i].obj;
        build_arg(cmd, obj);
        list_add(inputs, (void *)obj);
    }
}

static void build_plan_link_cmd(build_plan_t *plan, build_artifact_entry_t *entry, compiler_command_t *cmd, list_t *inputs, word *closure) {
    project_config_t *config = &plan->layout.config;
    build_targets_t *targets = &plan->layout.targets;
    build_target_t *target = &targets->items[entry->target];

    compiler_command_init(cmd, 16 + target->sources.count);

    if (entry->artifact.kind == BUILD_ARTIFACT_LIB) {
        build_arg(cmd, plan->toolchain.ar);
        build_arg(cmd, "rcs");
        build_arg(cmd, entry->artifact.path);
        build_plan_link_objects(plan, target, cmd, inputs);
        return;
    }

    word count = build_target_closure(targets, entry->target, true, closure);

    // Link with the C++ compiler if any of the linked sources is C++.
    bool cxx = build_target_has_cxx(target);
    for (word i = 0; i < count; i++) {
        build_target_t *dep = &targets->items[closure[i]];
        cxx = cxx || (dep->type != BUILD_TARGET_SHARED_LIBRARY && build_target_has_cxx(dep));
    }

    build_arg(cmd, cxx ? plan->toolchain.cxx : plan->toolchain.cc);
    if (entry->artifact.kind == BUILD_ARTIFACT_SHARED) {
        const char *file_name = strrchr(entry->artifact.path, '/') + 1;
#if defined(__APPLE__)
        build_arg(cmd, "-dynamiclib");
        build_arg(cmd, build_own(plan, strutils_format("-Wl,-install_name,@rpath/%s", file_name)));
#elif defined(_WIN32)
        build_arg(cmd, "-shared");
        (void)file_name;
#else
        build_arg(cmd, "-shared");
        build_arg(cmd, build_own(plan, strutils_format("-Wl,-soname,%s", file_name)));
#endif
    }

    build_plan_link_objects(plan, target, cmd, inputs);

    bool uses_shared = false;
    for (word i = 0; i < count; i++) {
        build_target_t *dep = &targets->items[closure[i]];
        if (dep->type == BUILD_TARGET_OBJECTS || dep->link_objects) {
            build_plan_link_objects(plan, dep, cmd, inputs);
            continue;
        }

        uses_shared = uses_shared || dep->type == BUILD_TARGET_SHARED_LIBRARY;
        build_artifact_kind_t kind = dep->type == BUILD_TARGET_SHARED_LIBRARY ? BUILD_ARTIFACT_SHARED : BUILD_ARTIFACT_LIB;
        const char *path = build_own(plan, build_plan_artifact_path(plan, dep, kind));
        build_arg(cmd, path);
        list_add(inputs, (void *)path);
    }

    build_arg(cmd, "-o");
    build_arg(cmd, entry->artifact.path);

    // The project's shared libraries are in build/<profile>, next to what uses them; tests are one level down.
    if (uses_shared) {
        bool in_tests = entry->artifact.kind == BUILD_ARTIFACT_TEST;
#if defined(__APPLE__)
        build_arg(cmd, in_tests ? "-Wl,-rpath,@loader_path/.." : "-Wl,-rpath,@loader_path");
#elif !defined(_WIN32)
        build_arg(cmd, in_tests ? "-Wl,-rpath,$ORIGIN/.." : "-Wl,-rpath,$ORIGIN");
#else
        (void)in_tests; // Windows finds the DLLs through PATH (see idl test)
#endif
    }

    build_arg_list(plan, cmd, &config->build.ldflags, nullptr);
    build_arg_list(plan, cmd, &target->ldflags, nullptr);
    build_arg_list(plan, cmd, &plan->toolchain.ldflags, nullptr);
    build_arg_list(plan, cmd, &target->libs, "-l");
    // A static library cannot carry the system libraries it needs: whoever links it does.
    for (word i = 0; i < count; i++) {
        build_target_t *dep = &targets->items[closure[i]];
        if (dep->type != BUILD_TARGET_SHARED_LIBRARY)
            build_arg_list(plan, cmd, &dep->libs, "-l");
    }
    build_arg_list(plan, cmd, &config->build.libs, "-l");
}

static bool build_plan_link_dirty(build_plan_t *plan, const char *path, const char *stamp, compiler_command_t *cmd, list_t *inputs) {
    int64 out_mtime = platform_file_mtime(path);
    if (out_mtime < 0 || !build_stamp_matches(plan, stamp, cmd))
        return true;

    for (list_item_t *item = inputs->head; item; item = item->next) {
        if (platform_file_mtime((const char *)item->value) > out_mtime)
            return true;
    }
    return false;
}

/* Links in waves by target level: a library is ready before whatever links it. */
static bool build_plan_link(build_plan_t *plan, build_artifact_entry_t *entries, word *linked) {
    build_targets_t *targets = &plan->layout.targets;
    word count = plan->artifact_count;
    compiler_command_t *cmds = (compiler_command_t *)calloc(count ? count : 1, sizeof(compiler_command_t));
    list_t *inputs = (list_t *)calloc(count ? count : 1, sizeof(list_t));
    process_job_t *jobs = (process_job_t *)calloc(count ? count : 1, sizeof(process_job_t));
    char **stamps = (char **)calloc(count ? count : 1, sizeof(char *));
    word *job_entry = (word *)calloc(count ? count : 1, sizeof(word));
    word *closure = (word *)calloc(targets->count ? targets->count : 1, sizeof(word));
    if (!cmds || !inputs || !jobs || !stamps || !job_entry || !closure) {
        LOG_FATAL_NOT_ENOUGH_MEMORY();
        free(cmds);
        free(inputs);
        free(jobs);
        free(stamps);
        free(job_entry);
        free(closure);
        return false;
    }

    static const char *kind_names[] = { "exe", "bin", "lib", "test", "shared" };
    word max_level = 0;

    for (word i = 0; i < count; i++) {
        build_artifact_entry_t *entry = &entries[i];
        list_init(&inputs[i], nullptr);
        build_plan_link_cmd(plan, entry, &cmds[i], &inputs[i], closure);
        stamps[i] = strutils_format("%s/obj/.link/%s-%s.cmd", plan->out_dir, kind_names[entry->artifact.kind], entry->artifact.name);
        if (targets->items[entry->target].level > max_level)
            max_level = targets->items[entry->target].level;
    }

    bool result = true;
    *linked = 0;

    for (word level = 0; result && level <= max_level; level++) {
        word job_count = 0;
        memset(jobs, 0, count * sizeof(process_job_t));

        for (word i = 0; i < count; i++) {
            build_artifact_entry_t *entry = &entries[i];
            if (targets->items[entry->target].level != level ||
                !build_plan_link_dirty(plan, entry->artifact.path, stamps[i], &cmds[i], &inputs[i]))
                continue;

            build_make_parent_dirs(entry->artifact.path);
            build_make_parent_dirs(stamps[i]);
            if (entry->artifact.kind == BUILD_ARTIFACT_LIB)
                platform_remove_tree(entry->artifact.path); // ar rcs would keep stale members

            const char *verb = entry->artifact.kind == BUILD_ARTIFACT_LIB ? "Archiving" : "Linking";
            jobs[job_count].label = build_own(plan, strutils_format("  %s %s", verb, entry->artifact.path));
            jobs[job_count].cmd = &cmds[i];
            job_entry[job_count++] = i;
        }

        result = process_runner_run(jobs, job_count, 0);

        for (word j = 0; j < job_count; j++) {
            if (jobs[j].exit_status == 0)
                build_stamp_write(plan, stamps[job_entry[j]], &cmds[job_entry[j]]);
        }
        *linked += job_count;
    }

    for (word i = 0; i < count; i++) {
        compiler_command_clear(&cmds[i]);
        list_clear(&inputs[i]);
        free(stamps[i]);
    }
    free(cmds);
    free(inputs);
    free(jobs);
    free(stamps);
    free(job_entry);
    free(closure);
    return result;
}

static bool build_plan_compile(build_plan_t *plan, word *compiled) {
    process_job_t *jobs = (process_job_t *)calloc(plan->unit_count ? plan->unit_count : 1, sizeof(process_job_t));
    build_unit_t **job_unit = (build_unit_t **)calloc(plan->unit_count ? plan->unit_count : 1, sizeof(build_unit_t *));
    if (!jobs || !job_unit) {
        LOG_FATAL_NOT_ENOUGH_MEMORY();
        free(jobs);
        free(job_unit);
        return false;
    }

    word job_count = 0;
    for (word i = 0; i < plan->unit_count; i++) {
        build_unit_t *unit = &plan->units[i];

        int64 obj_mtime = platform_file_mtime(unit->obj);
        unit->dirty = obj_mtime < 0 ||
                      !build_stamp_matches(plan, unit->stamp, &unit->cmd) ||
                      build_deps_changed(unit->dep, obj_mtime);
        if (!unit->dirty)
            continue;

        build_make_parent_dirs(unit->obj);
        // In project.yml a source can be in several targets: the message names the target.
        jobs[job_count].label = plan->layout.has_targets
                                    ? build_own(plan, strutils_format("  Compiling %s (%s)", unit->source->path, plan->layout.targets.items[unit->target].name))
                                    : build_own(plan, strutils_format("  Compiling %s", unit->source->path));
        jobs[job_count].cmd = &unit->cmd;
        job_unit[job_count++] = unit;
    }

    bool result = process_runner_run(jobs, job_count, 0);

    for (word j = 0; j < job_count; j++) {
        if (jobs[j].exit_status == 0)
            build_stamp_write(plan, job_unit[j]->stamp, &job_unit[j]->cmd);
    }
    *compiled = job_count;

    free(jobs);
    free(job_unit);
    return result;
}

/*****************************************************************************/
/* compile_commands.json                                                     */
/*****************************************************************************/

static void build_json_string(FILE *f, const char *str) {
    fputc('"', f);
    for (const char *p = str; *p; p++) {
        switch (*p) {
            case '"':  fputs("\\\"", f); break;
            case '\\': fputs("\\\\", f); break;
            case '\n': fputs("\\n", f); break;
            case '\r': fputs("\\r", f); break;
            case '\t': fputs("\\t", f); break;
            default:
                if ((unsigned char)*p < 0x20)
                    fprintf(f, "\\u%04x", (unsigned char)*p);
                else
                    fputc(*p, f);
                break;
        }
    }
    fputc('"', f);
}

/* Used by clangd and the C/C++ editor extensions. */
static void build_plan_write_compile_commands(build_plan_t *plan) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == nullptr || !platform_make_dirs(BUILD_OUT_DIR))
        return;

    FILE *f = fopen(BUILD_COMPILE_COMMANDS, "wb");
    if (!f) {
        log_warn("Could not write %s.", BUILD_COMPILE_COMMANDS);
        return;
    }

    fputs("[\n", f);
    for (word i = 0; i < plan->unit_count; i++) {
        build_unit_t *unit = &plan->units[i];

        fputs("  {\n    \"directory\": ", f);
        build_json_string(f, cwd);
        fputs(",\n    \"file\": ", f);
        build_json_string(f, unit->source->path);
        fputs(",\n    \"output\": ", f);
        build_json_string(f, unit->obj);
        fputs(",\n    \"arguments\": [", f);
        for (word a = 0; a < unit->cmd.count; a++) {
            if (a > 0)
                fputs(", ", f);
            build_json_string(f, unit->cmd.args[a]);
        }
        fprintf(f, "]\n  }%s\n", i + 1 < plan->unit_count ? "," : "");
    }
    fputs("]\n", f);
    fclose(f);
}

/*****************************************************************************/
/* Target selection (idl build <target>...)                                  */
/*****************************************************************************/

static bool build_plan_has_selection(build_plan_t *plan) {
    return plan->options.targets && plan->options.targets->count > 0;
}

/* Lists the names that can be asked for, for the error message. */
static char *build_plan_target_names(build_targets_t *targets) {
    stream_t out = {0};
    stream_init(&out, 64);
    for (word i = 0; i < targets->count; i++) {
        build_target_t *target = &targets->items[i];
        if (target->type == BUILD_TARGET_OBJECTS)
            continue;

        bool repeated = false; // an executable and a library may share a name
        for (word j = 0; j < i && !repeated; j++)
            repeated = strcmp(targets->items[j].name, target->name) == 0;
        if (!repeated)
            stream_write_string(&out, "%s%s", stream_get_position(&out) ? ", " : "", target->name);
    }
    stream_write(&out, (const byte *)"", 1);
    return (char *)out.data;
}

/* Marks the targets asked for by name (all of them without names) and those they link. */
static bool build_plan_select(build_plan_t *plan) {
    build_targets_t *targets = &plan->layout.targets;
    plan->wanted = (byte *)calloc(targets->count ? targets->count : 1, 1);
    word *closure = (word *)calloc(targets->count ? targets->count : 1, sizeof(word));
    if (!plan->wanted || !closure) {
        LOG_FATAL_NOT_ENOUGH_MEMORY();
        free(closure);
        return false;
    }

    if (!build_plan_has_selection(plan)) {
        memset(plan->wanted, 2, targets->count);
        free(closure);
        return true;
    }

    for (list_item_t *item = plan->options.targets->head; item; item = item->next) {
        const char *name = (const char *)item->value;
        bool found = false;

        for (word i = 0; i < targets->count; i++) {
            if (targets->items[i].type == BUILD_TARGET_OBJECTS || strcmp(targets->items[i].name, name) != 0)
                continue;

            found = true;
            plan->wanted[i] = 2;
            word count = build_target_closure(targets, i, false, closure);
            for (word c = 0; c < count; c++) {
                if (plan->wanted[closure[c]] == 0)
                    plan->wanted[closure[c]] = 1;
            }
        }

        if (!found) {
            char *names = build_plan_target_names(targets);
            log_error("Target '%s' not found. Targets of the project: %s.", name, names[0] ? names : "(none)");
            free(names);
            free(closure);
            return false;
        }
    }

    free(closure);
    return true;
}

static bool build_plan_uses_lang(build_plan_t *plan, build_lang_t lang) {
    build_targets_t *targets = &plan->layout.targets;
    for (word t = 0; t < targets->count; t++) {
        build_sources_t *sources = &targets->items[t].sources;
        for (word i = 0; plan->wanted[t] && i < sources->count; i++) {
            if (sources->items[i].lang == lang)
                return true;
        }
    }
    return false;
}

/*****************************************************************************/
/* API                                                                       */
/*****************************************************************************/

bool build_plan_run(build_plan_t *plan, const build_options_t *options) {
    RETURN_VAL_IF_FAIL(plan, false);
    RETURN_VAL_IF_FAIL(options, false);

    memset(plan, 0, sizeof(build_plan_t));
    list_init(&plan->strings, free);
    plan->options = *options;
    plan->profile = options->release ? "release" : "debug";
    plan->out_dir = strutils_format("%s/%s", BUILD_OUT_DIR, plan->profile);

    if (!build_layout_load(&plan->layout, options->with_tests))
        return false;

    if (!build_plan_select(plan))
        return false;

    build_layout_t *layout = &plan->layout;
    bool need_c = build_plan_uses_lang(plan, BUILD_LANG_C);
    bool need_cxx = build_plan_uses_lang(plan, BUILD_LANG_CXX);

    // The variables of envs: go into idl's own environment, so every process it starts from
    // here on (compiler, linker, pkg-config, and the program of run/test) inherits them.
    if (!build_env_apply(&layout->config.envs, &plan->env_stamp))
        return false;

    if (!build_toolchain_init(&plan->toolchain, need_c, need_cxx) ||
        !build_toolchain_resolve_deps(&plan->toolchain, &layout->config.dependencies) ||
        !build_plan_create_units(plan))
        return false;

    // With target names only part of the project is compiled: the file keeps describing all of it.
    if (!build_plan_has_selection(plan))
        build_plan_write_compile_commands(plan);

    printf("Building %s (%s)\n", layout->name, plan->profile);
    fflush(stdout);

    word compiled = 0;
    word linked = 0;
    if (!build_plan_compile(plan, &compiled)) {
        log_error("Compilation failed.");
        return false;
    }

    if (!build_plan_link_artifacts(plan, &linked)) {
        log_error("Linking failed.");
        return false;
    }

    if (compiled == 0 && linked == 0)
        printf("Nothing to do: everything is up to date.\n");

    for (word i = 0; i < plan->artifact_count; i++) {
        if (plan->artifacts[i].kind != BUILD_ARTIFACT_TEST)
            printf("Finished: %s\n", plan->artifacts[i].path);
    }
    return true;
}

void build_plan_clear(build_plan_t *plan) {
    RETURN_IF_FAIL(plan);

    for (word i = 0; i < plan->unit_count; i++) {
        free(plan->units[i].obj);
        free(plan->units[i].dep);
        free(plan->units[i].stamp);
        compiler_command_clear(&plan->units[i].cmd);
    }
    free(plan->units);

    for (word i = 0; plan->artifacts && i < plan->artifact_count; i++) {
        free(plan->artifacts[i].name);
        free(plan->artifacts[i].path);
    }
    free(plan->artifacts);

    list_clear(&plan->strings);
    build_layout_clear(&plan->layout);
    build_toolchain_clear(&plan->toolchain);
    free(plan->out_dir);
    free(plan->env_stamp);
    free(plan->wanted);
    memset(plan, 0, sizeof(build_plan_t));
}

build_artifact_t *build_plan_find_artifact(build_plan_t *plan, build_artifact_kind_t kind, const char *name) {
    RETURN_VAL_IF_FAIL(plan, nullptr);

    for (word i = 0; i < plan->artifact_count; i++) {
        build_artifact_t *artifact = &plan->artifacts[i];
        if (artifact->kind == kind && (!name || strcmp(artifact->name, name) == 0))
            return artifact;
    }
    return nullptr;
}

bool build_enter_project_dir(cmd_args_t *args) {
    RETURN_VAL_IF_FAIL(args, false);

    const char *dir = cmd_args_get_value(args, "project");
    if (!dir)
        dir = cmd_args_get_value(args, "p");
    if (!dir)
        return true;

    int r = uv_chdir(dir);
    if (r) {
        log_error("Could not enter %s: %s", dir, uv_strerror(r));
        return false;
    }
    return true;
}
