#include <stdio.h>
#include "soma.h"

int main(void) {
    int ok = soma(2, 3) == 5;
    printf("soma(2, 3) == 5: %s\n", ok ? "sim" : "nao");
    return ok ? 0 : 1;
}
