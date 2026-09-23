#include <stdio.h>
#include <core/core.h>

int args_count(int argc);

int main(int argc, char **argv) {
    (void)argv;
    printf("core=%d extra=%d os=%s args=%d\n", core_value(), core_extra(), core_os(), args_count(argc));
    return 0;
}
