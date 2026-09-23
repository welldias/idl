#include "idl.h"

int handle_param_export(int argc, char *argv[]) {
    printf("You type param");
    for (int i = 1; i < argc; i++) {
        printf(" %s", argv[i]);
    }
    printf("\n");

    return 0;
}
