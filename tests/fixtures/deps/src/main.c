#include <math.h>
#include <stdio.h>
#include <extra.h>

int main(void) {
    volatile double x = 16.0; /* volatile: forces the call to libm's sqrt */
    printf("sqrt=%.1f version=%d extra=%s\n", sqrt(x), VERSION, EXTRA_NAME);
    return 0;
}
