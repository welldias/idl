#include <stdio.h>
#include "soma.h"

int main(void) {
    int ok = soma(1, 1) == 3;
    printf("soma(1, 1) == 3: %s\n", ok ? "sim" : "nao");
    return ok ? 0 : 1;
}
