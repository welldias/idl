#include <stdio.h>
#include "idl.h"
#include "help.h"

int handle_param_help(int argc, char *argv[]) {
    printf("%s", HELP_TEXT);

    return 0;
}
