#include "build_plan.h"
#include "compiler_command.h"
#include "process_runner.h"

#include <uv.h>

#define BUILD_COMPILE_COMMANDS BUILD_OUT_DIR "/compile_commands.json"

struct build_unit_t {
    build_source_t *source;
    char *obj;      // build/<perfil>/obj/<fonte>.o
    char *dep;      // build/<perfil>/obj/<fonte>.d   (gerado pelo compilador com -MMD)
    char *stamp;    // build/<perfil>/obj/<fonte>.cmd (comando usado na última compilação)
    compiler_command_t cmd;
    bool dirty;
};

/* Guarda a string no plano para ser liberada junto com ele. */
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
/* Build incremental                                                         */
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

/* O comando mudou desde o último build (flags, compilador...)? */
static bool build_stamp_matches(const char *stamp, compiler_command_t *cmd) {
    char *content = build_read_file(stamp);
    char *joined = build_cmd_join(cmd);
    bool result = content && joined && strcmp(content, joined) == 0;
    free(content);
    free(joined);
    return result;
}

static void build_stamp_write(const char *stamp, compiler_command_t *cmd) {
    char *joined = build_cmd_join(cmd);
    FILE *f = joined ? fopen(stamp, "wb") : nullptr;
    if (f) {
        fputs(joined, f);
        fclose(f);
    }
    free(joined);
}

/* Lê a primeira regra do arquivo .d ("obj: fonte header1 header2 ...") e
   verifica se alguma dependência sumiu ou é mais nova que o objeto. */
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
            break; // fim da primeira regra; as seguintes são alvos falsos do -MP

        const char *start = p;
        word len = 0;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') {
            if (*p == '\\' && (p[1] == '\n' || p[1] == '\r'))
                break;
            if ((*p == '\\' && p[1] == ' ') || (*p == '$' && p[1] == '$'))
                p++; // espaço escapado ou "$$"
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
/* Montagem dos comandos                                                     */
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
        log_warn("%s '%s' não reconhecido; nenhum -std será usado.", key, value);
        free(flag);
        return nullptr;
    }
    return flag;
}

static void build_plan_compile_cmd(build_plan_t *plan, build_unit_t *unit) {
    compiler_command_t *cmd = &unit->cmd;
    project_config_t *config = &plan->layout.config;
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

    build_arg(cmd, "-I" BUILD_SRC_DIR);
    if (plan->layout.has_include_dir)
        build_arg(cmd, "-I" BUILD_INCLUDE_DIR);
    build_arg_list(plan, cmd, &config->build.include_dirs, "-I");
    build_arg_list(plan, cmd, &config->build.defines, "-D");
    build_arg_list(plan, cmd, &plan->toolchain.cflags, nullptr);
    build_arg_list(plan, cmd, cxx ? &config->build.cxxflags : &config->build.cflags, nullptr);

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
    build_layout_t *layout = &plan->layout;
    build_sources_t *groups[] = { &layout->main, &layout->lib, &layout->bins, &layout->tests };
    word group_count = plan->options.with_tests ? 4 : 3;

    word total = 0;
    for (word g = 0; g < group_count; g++)
        total += groups[g]->count;

    plan->units = (build_unit_t *)calloc(total ? total : 1, sizeof(build_unit_t));
    if (!plan->units) {
        LOG_FATAL_NOT_ENOUGH_MEMORY();
        return false;
    }

    for (word g = 0; g < group_count; g++) {
        for (word i = 0; i < groups[g]->count; i++) {
            build_unit_t *unit = &plan->units[plan->unit_count++];
            unit->source = &groups[g]->items[i];
            unit->obj = strutils_format("%s/obj/%s.o", plan->out_dir, unit->source->path);
            unit->dep = strutils_format("%s/obj/%s.d", plan->out_dir, unit->source->path);
            unit->stamp = strutils_format("%s/obj/%s.cmd", plan->out_dir, unit->source->path);
            build_plan_compile_cmd(plan, unit);
        }
    }
    return true;
}

/* Índices das unidades: [main][lib...][bins...][tests...] */
static word build_plan_lib_first(build_plan_t *plan) {
    return plan->layout.main.count;
}

static word build_plan_bins_first(build_plan_t *plan) {
    return plan->layout.main.count + plan->layout.lib.count;
}

static word build_plan_tests_first(build_plan_t *plan) {
    return build_plan_bins_first(plan) + plan->layout.bins.count;
}

/* Cada artefato usa todos os fontes da biblioteca interna (src/ sem main e sem bin/)
   mais, opcionalmente, o próprio fonte (main, bin ou teste). */
typedef struct {
    build_artifact_t artifact;
    bool has_unit;
    word unit;
} build_artifact_entry_t;

static void build_plan_add_artifact(build_plan_t *plan, build_artifact_entry_t *entries, build_artifact_kind_t kind,
                                    const char *name, char *path, bool has_unit, word unit) {
    build_artifact_entry_t *entry = &entries[plan->artifact_count++];
    entry->artifact.kind = kind;
    entry->artifact.name = strutils_strndup(name, strlen(name));
    entry->artifact.path = path;
    entry->has_unit = has_unit;
    entry->unit = unit;
}

static bool build_plan_link(build_plan_t *plan, build_artifact_entry_t *entries, word *linked);

static bool build_plan_link_artifacts(build_plan_t *plan, word *linked) {
    build_layout_t *layout = &plan->layout;
    word max = 1 + layout->bins.count + layout->tests.count;

    build_artifact_entry_t *entries = (build_artifact_entry_t *)calloc(max, sizeof(build_artifact_entry_t));
    if (!entries) {
        LOG_FATAL_NOT_ENOUGH_MEMORY();
        return false;
    }

    if (layout->main.count) {
        build_plan_add_artifact(plan, entries, BUILD_ARTIFACT_EXE, layout->name,
                                strutils_format("%s/%s%s", plan->out_dir, layout->name, EXE_EXTENSION), true, 0);
    } else if (layout->lib.count) {
        build_plan_add_artifact(plan, entries, BUILD_ARTIFACT_LIB, layout->name,
                                strutils_format("%s/lib%s.a", plan->out_dir, layout->name), false, 0);
    }

    for (word i = 0; i < layout->bins.count; i++) {
        const char *stem = layout->bins.items[i].stem;
        build_plan_add_artifact(plan, entries, BUILD_ARTIFACT_BIN, stem,
                                strutils_format("%s/%s%s", plan->out_dir, stem, EXE_EXTENSION),
                                true, build_plan_bins_first(plan) + i);
    }

    for (word i = 0; plan->options.with_tests && i < layout->tests.count; i++) {
        const char *stem = layout->tests.items[i].stem;
        build_plan_add_artifact(plan, entries, BUILD_ARTIFACT_TEST, stem,
                                strutils_format("%s/tests/%s%s", plan->out_dir, stem, EXE_EXTENSION),
                                true, build_plan_tests_first(plan) + i);
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

static void build_plan_link_cmd(build_plan_t *plan, build_artifact_entry_t *entry, compiler_command_t *cmd) {
    project_config_t *config = &plan->layout.config;
    word lib_first = build_plan_lib_first(plan);
    word lib_count = plan->layout.lib.count;

    compiler_command_init(cmd, 16 + lib_count);

    if (entry->artifact.kind == BUILD_ARTIFACT_LIB) {
        build_arg(cmd, plan->toolchain.ar);
        build_arg(cmd, "rcs");
        build_arg(cmd, entry->artifact.path);
        for (word i = 0; i < lib_count; i++)
            build_arg(cmd, plan->units[lib_first + i].obj);
        return;
    }

    // Linka com o compilador C++ se algum dos fontes for C++.
    bool cxx = entry->has_unit && plan->units[entry->unit].source->lang == BUILD_LANG_CXX;
    for (word i = 0; i < lib_count; i++)
        cxx = cxx || plan->units[lib_first + i].source->lang == BUILD_LANG_CXX;

    build_arg(cmd, cxx ? plan->toolchain.cxx : plan->toolchain.cc);
    if (entry->has_unit)
        build_arg(cmd, plan->units[entry->unit].obj);
    for (word i = 0; i < lib_count; i++)
        build_arg(cmd, plan->units[lib_first + i].obj);
    build_arg(cmd, "-o");
    build_arg(cmd, entry->artifact.path);
    build_arg_list(plan, cmd, &config->build.ldflags, nullptr);
    build_arg_list(plan, cmd, &plan->toolchain.ldflags, nullptr);
    build_arg_list(plan, cmd, &config->build.libs, "-l");
}

static bool build_plan_link(build_plan_t *plan, build_artifact_entry_t *entries, word *linked) {
    word count = plan->artifact_count;
    compiler_command_t *cmds = (compiler_command_t *)calloc(count ? count : 1, sizeof(compiler_command_t));
    process_job_t *jobs = (process_job_t *)calloc(count ? count : 1, sizeof(process_job_t));
    char **stamps = (char **)calloc(count ? count : 1, sizeof(char *));
    word *job_entry = (word *)calloc(count ? count : 1, sizeof(word));
    if (!cmds || !jobs || !stamps || !job_entry) {
        LOG_FATAL_NOT_ENOUGH_MEMORY();
        free(cmds);
        free(jobs);
        free(stamps);
        free(job_entry);
        return false;
    }

    static const char *kind_names[] = { "exe", "bin", "lib", "test" };
    word lib_first = build_plan_lib_first(plan);
    word job_count = 0;

    for (word i = 0; i < count; i++) {
        build_artifact_entry_t *entry = &entries[i];
        build_plan_link_cmd(plan, entry, &cmds[i]);
        stamps[i] = strutils_format("%s/obj/.link/%s-%s.cmd", plan->out_dir, kind_names[entry->artifact.kind], entry->artifact.name);

        int64 out_mtime = platform_file_mtime(entry->artifact.path);
        bool dirty = out_mtime < 0 || !build_stamp_matches(stamps[i], &cmds[i]);
        for (word u = 0; !dirty && u < plan->layout.lib.count; u++)
            dirty = platform_file_mtime(plan->units[lib_first + u].obj) > out_mtime;
        if (!dirty && entry->has_unit)
            dirty = platform_file_mtime(plan->units[entry->unit].obj) > out_mtime;

        if (!dirty)
            continue;

        build_make_parent_dirs(entry->artifact.path);
        build_make_parent_dirs(stamps[i]);
        if (entry->artifact.kind == BUILD_ARTIFACT_LIB)
            platform_remove_tree(entry->artifact.path); // ar rcs mantém membros antigos

        const char *verb = entry->artifact.kind == BUILD_ARTIFACT_LIB ? "Arquivando" : "Linkando";
        jobs[job_count].label = build_own(plan, strutils_format("  %s %s", verb, entry->artifact.path));
        jobs[job_count].cmd = &cmds[i];
        job_entry[job_count++] = i;
    }

    bool result = process_runner_run(jobs, job_count, 0);

    for (word j = 0; j < job_count; j++) {
        if (jobs[j].exit_status == 0)
            build_stamp_write(stamps[job_entry[j]], &cmds[job_entry[j]]);
    }
    *linked = job_count;

    for (word i = 0; i < count; i++) {
        compiler_command_clear(&cmds[i]);
        free(stamps[i]);
    }
    free(cmds);
    free(jobs);
    free(stamps);
    free(job_entry);
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
                      !build_stamp_matches(unit->stamp, &unit->cmd) ||
                      build_deps_changed(unit->dep, obj_mtime);
        if (!unit->dirty)
            continue;

        build_make_parent_dirs(unit->obj);
        jobs[job_count].label = build_own(plan, strutils_format("  Compilando %s", unit->source->path));
        jobs[job_count].cmd = &unit->cmd;
        job_unit[job_count++] = unit;
    }

    bool result = process_runner_run(jobs, job_count, 0);

    for (word j = 0; j < job_count; j++) {
        if (jobs[j].exit_status == 0)
            build_stamp_write(job_unit[j]->stamp, &job_unit[j]->cmd);
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

/* Usado por clangd e pelas extensões C/C++ dos editores. */
static void build_plan_write_compile_commands(build_plan_t *plan) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == nullptr || !platform_make_dirs(BUILD_OUT_DIR))
        return;

    FILE *f = fopen(BUILD_COMPILE_COMMANDS, "wb");
    if (!f) {
        log_warn("Não foi possível gravar %s.", BUILD_COMPILE_COMMANDS);
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

    build_layout_t *layout = &plan->layout;
    bool need_c = build_layout_uses_lang(layout, BUILD_LANG_C, options->with_tests);
    bool need_cxx = build_layout_uses_lang(layout, BUILD_LANG_CXX, options->with_tests);

    if (!build_toolchain_init(&plan->toolchain, need_c, need_cxx) ||
        !build_toolchain_resolve_deps(&plan->toolchain, &layout->config.dependencies) ||
        !build_plan_create_units(plan))
        return false;

    build_plan_write_compile_commands(plan);

    printf("Construindo %s (%s)\n", layout->name, plan->profile);
    fflush(stdout);

    word compiled = 0;
    word linked = 0;
    if (!build_plan_compile(plan, &compiled)) {
        log_error("Falha na compilação.");
        return false;
    }

    if (!build_plan_link_artifacts(plan, &linked)) {
        log_error("Falha na linkagem.");
        return false;
    }

    if (compiled == 0 && linked == 0)
        printf("Nada a fazer: tudo está atualizado.\n");

    for (word i = 0; i < plan->artifact_count; i++) {
        if (plan->artifacts[i].kind != BUILD_ARTIFACT_TEST)
            printf("Pronto: %s\n", plan->artifacts[i].path);
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

bool build_enter_project_dir(const char *dir) {
    if (!dir)
        return true;

    int r = uv_chdir(dir);
    if (r) {
        log_error("Não foi possível entrar em %s: %s", dir, uv_strerror(r));
        return false;
    }
    return true;
}
