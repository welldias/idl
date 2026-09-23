#include <math.h>
#include <stdio.h>
#include <extra.h>

int main(void) {
    volatile double x = 16.0; /* volatile: força a chamada a sqrt da libm */
    printf("raiz=%.1f versao=%d extra=%s\n", sqrt(x), VERSAO, EXTRA_NOME);
    return 0;
}
