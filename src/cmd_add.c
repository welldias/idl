#include "project_file.h"

int handle_param_add(int argc, char *argv[]) {

    if (project_file_exist() == false){
        fprintf(stderr, "Project file not found\n");   
        return 1;
    }

    if (argc < 3) {
        fprintf(stderr, "Usage: idl add <dependency>\n");
        return 1;
    }

    project_config_t config = {0};
    project_file_config_init(&config);

    bool result = project_file_read(&config) &&
                  project_file_dependency_add(&config, argv[2]) &&
                  project_file_save(&config);

    project_file_config_clean(&config);
    return result ? 0 : 1;
}
