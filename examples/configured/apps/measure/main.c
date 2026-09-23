#include <stdio.h>
#include <stdlib.h>

#include <geometry/geometry.h>

#include "parse.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: measure x,y x,y ...\n");
        return 1;
    }

    size_t count = (size_t)(argc - 1);
    point_t *points = (point_t *)calloc(count, sizeof(point_t));
    if (!points)
        return 1;

    for (size_t i = 0; i < count; i++) {
        if (!parse_point(argv[i + 1], &points[i])) {
            fprintf(stderr, "measure: invalid point '%s' (expected x,y)\n", argv[i + 1]);
            free(points);
            return 1;
        }
    }

    char text[128];
    geometry_describe(text, sizeof(text), points, count);
    printf("%s\n", text);
    free(points);
    return 0;
}
