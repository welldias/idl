#include "project_file.h"
#include "build_layout.h"

#define DIR_NAME_LEN 1024
#define README_MD_NAME "README.md"
#define MAIN_C_NAME BUILD_SRC_DIR "/main.c"

static const char *main_candidates[] = {
    BUILD_SRC_DIR "/main.c", BUILD_SRC_DIR "/main.cpp", BUILD_SRC_DIR "/main.cc",
    BUILD_SRC_DIR "/main.cxx", BUILD_SRC_DIR "/main.c++",
};

static bool init_file_exists(const char *path) {
    return platform_file_mtime(path) >= 0;
}

/* Cria o arquivo somente se ele ainda não existir. */
static bool init_write_new_file(const char *path, const char *content) {
    if (init_file_exists(path))
        return true;

    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "Error on create %s: %s\n", path, strerror(errno));
        return false;
    }
    fputs(content, f);
    return fclose(f) == 0;
}

static bool init_write_main(const char *project_name) {
    for (word i = 0; i < SIZE_OF_ARRAY(main_candidates); i++) {
        if (init_file_exists(main_candidates[i]))
            return true;
    }

    // O nome do projeto vai dentro de uma string C: escapa aspas e barras.
    char name[DIR_NAME_LEN * 2];
    word len = 0;
    for (const char *p = project_name; *p && len < sizeof(name) - 2; p++) {
        if (*p == '"' || *p == '\\')
            name[len++] = '\\';
        name[len++] = *p;
    }
    name[len] = '\0';

    char *content = strutils_format(
        "#include <stdio.h>\n"
        "\n"
        "int main(void) {\n"
        "    puts(\"Hello from %s!\");\n"
        "    return 0;\n"
        "}\n",
        name);
    bool result = content && init_write_new_file(MAIN_C_NAME, content);
    free(content);
    return result;
}

int handle_param_init(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    if (project_file_exist()) {
        fprintf(stderr, "Project is already initialized (%s exists)\n", PROJECT_FILE_NAME);
        return 1;
    }

    char cwd[DIR_NAME_LEN];
    if (getcwd(cwd, DIR_NAME_LEN) == NULL) {
        fprintf(stderr, "Can't get current working directory\n");
        snprintf(cwd, DIR_NAME_LEN, "%s", "program");
    }

    char *dir_name = cwd;
    for (char *p = cwd; *p; p++) {
        if (*p == '/' || *p == '\\') {
            dir_name = p + 1;
        }
    }

    project_config_t config = {0};
    project_file_config_init(&config);

    project_file_name_set(&config, dir_name);
    project_file_version_set(&config, "0.1.0");
    project_file_description_set(&config, "");
    project_file_requires_c_set(&config, "C23");

    bool saved = project_file_save(&config);
    project_file_config_clean(&config);
    if (!saved)
        return 1;

    if (!init_write_new_file(README_MD_NAME, ""))
        return 1;

    if (!platform_make_dirs(BUILD_SRC_DIR)) {
        fprintf(stderr, "Error on create %s directory\n", BUILD_SRC_DIR);
        return 1;
    }

    if (!init_write_main(dir_name))
        return 1;

    printf("Project '%s' initialized successfully\n", dir_name);

    return 0;
}
