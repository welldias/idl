#include <stdio.h>
#include <stdlib.h>
#include <calc/calc.h>

int main(int argc, char **argv) {
    int total = 0;
    for (int i = 1; i < argc; i++)
        total = calc_somar(total, atoi(argv[i]));
    printf("%s total=%d\n", calc_nome(), total);
    return 0;
}
