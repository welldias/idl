#include <stdio.h>

#include <shapes/shapes.h>

static int failures = 0;

static void check(const char *what, double got, double expected) {
    double diff = got > expected ? got - expected : expected - got;
    if (diff > 1e-9) {
        printf("FAIL %s: got %f, expected %f\n", what, got, expected);
        failures++;
    }
}

int main(void) {
    check("circle area r=1", shapes_circle_area(1.0), 3.14159265358979323846);
    check("circle perimeter r=0", shapes_circle_perimeter(0.0), 0.0);
    check("rectangle area 3x4", shapes_rectangle_area(3.0, 4.0), 12.0);
    check("rectangle perimeter 3x4", shapes_rectangle_perimeter(3.0, 4.0), 14.0);

    printf("%s\n", failures ? "shapes: FAILED" : "shapes: ok");
    return failures ? 1 : 0;
}
