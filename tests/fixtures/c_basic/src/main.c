#include <stdio.h>
#include "util.h"

int main(int argc, char **argv) {
#ifdef NDEBUG
    const char *profile = "release";
#else
    const char *profile = "debug";
#endif
    printf("sum=%d profile=%s args=%d\n", sum(2, 3), profile, argc - 1);
    for (int i = 1; i < argc; i++)
        printf("arg[%d]=%s\n", i, argv[i]);
    return 0;
}
