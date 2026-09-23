#include <stdio.h>

#include <geometry/geometry.h>
#include <textutil.h>

#include "internal/checks.h"

double geometry_perimeter(const point_t *points, size_t count) {
    if (count < GEOMETRY_MIN_POINTS)
        return 0.0;

    double total = 0.0;
    for (size_t i = 0; i < count; i++)
        total += geometry_distance(points[i], points[(i + 1) % count]);
    return total;
}

void geometry_describe(char *out, size_t size, const point_t *points, size_t count) {
    snprintf(out, size, "%zu %s, perimeter %.2f", count, textutil_plural((int)count, "point", "points"),
             geometry_perimeter(points, count));
}
