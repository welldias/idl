#include <stdio.h>
#include <core/core.h>

int main(void) {
    int ok = core_value() == 19 && core_extra() == 5;
    printf("core: %s\n", ok ? "ok" : "wrong");
    return ok ? 0 : 1;
}
