#include <math.h>
#include <core/core.h>

int core_extra(void) {
    volatile double value = 25.0; // computed at run time, so libm is really needed
    return (int)sqrt(value);
}
