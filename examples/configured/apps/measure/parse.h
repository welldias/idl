#ifndef MEASURE_PARSE_H
#define MEASURE_PARSE_H

#include <stdbool.h>
#include <geometry/geometry.h>

/* Reads a point written as "x,y". */
bool parse_point(const char *text, point_t *point);

#endif
