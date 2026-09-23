#include <math.h>

#include <geometry/geometry.h>

double geometry_distance(point_t a, point_t b) {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return sqrt(dx * dx + dy * dy);
}
