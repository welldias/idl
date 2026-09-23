#include <stdio.h>

#include "parse.h"

bool parse_point(const char *text, point_t *point) {
    char extra;
    return sscanf(text, "%lf,%lf%c", &point->x, &point->y, &extra) == 2;
}
