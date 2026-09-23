#include <shapes/shapes.h>

double shapes_rectangle_area(double width, double height) {
    return width * height;
}

double shapes_rectangle_perimeter(double width, double height) {
    return 2.0 * (width + height);
}
