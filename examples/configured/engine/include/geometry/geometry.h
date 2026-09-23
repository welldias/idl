#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double x;
    double y;
} point_t;

double geometry_distance(point_t a, point_t b);

/* Perimeter of a closed polygon; 0 with fewer than 2 points. */
double geometry_perimeter(const point_t *points, size_t count);

/* Writes something like "4 points, perimeter 4.00" into <out>. */
void geometry_describe(char *out, size_t size, const point_t *points, size_t count);

#ifdef __cplusplus
}
#endif

#endif
