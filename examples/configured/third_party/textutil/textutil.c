#include "textutil.h"

const char *textutil_plural(int n, const char *one, const char *many) {
    return n == 1 ? one : many;
}
