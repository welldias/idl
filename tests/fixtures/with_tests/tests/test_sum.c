#include <stdio.h>
#include "sum.h"

int main(void) {
    int ok = sum(2, 3) == 5;
    printf("sum(2, 3) == 5: %s\n", ok ? "yes" : "no");
    return ok ? 0 : 1;
}
