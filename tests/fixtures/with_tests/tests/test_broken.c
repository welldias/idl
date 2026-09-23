#include <stdio.h>
#include "sum.h"

int main(void) {
    int ok = sum(1, 1) == 3;
    printf("sum(1, 1) == 3: %s\n", ok ? "yes" : "no");
    return ok ? 0 : 1;
}
