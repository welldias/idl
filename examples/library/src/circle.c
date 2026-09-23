#include <shapes/shapes.h>

#define SHAPES_PI 3.14159265358979323846

double shapes_circle_area(double radius) {
    return SHAPES_PI * radius * radius;
}

double shapes_circle_perimeter(double radius) {
    return 2.0 * SHAPES_PI * radius;
}
