#include <stdio.h>

#include <geometry/geometry.h>

static int failures = 0;

static void check(const char *what, double got, double expected) {
    double diff = got > expected ? got - expected : expected - got;
    if (diff > 1e-9) {
        printf("FAIL %s: got %f, expected %f\n", what, got, expected);
        failures++;
    }
}

int main(void) {
    point_t square[] = { { 0, 0 }, { 2, 0 }, { 2, 2 }, { 0, 2 } };
    point_t triangle[] = { { 0, 0 }, { 3, 0 }, { 3, 4 } };

    check("distance", geometry_distance(triangle[0], triangle[2]), 5.0);
    check("square", geometry_perimeter(square, 4), 8.0);
    check("triangle", geometry_perimeter(triangle, 3), 12.0);
    check("single point", geometry_perimeter(square, 1), 0.0);

    printf("%s\n", failures ? "perimeter: FAILED" : "perimeter: ok");
    return failures ? 1 : 0;
}
