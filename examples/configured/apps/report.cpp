// A C++ program using the C library.
#include <cstdio>
#include <vector>

#include <geometry/geometry.h>

static std::vector<point_t> square(double side) {
    return { { 0, 0 }, { side, 0 }, { side, side }, { 0, side } };
}

int main() {
    std::printf("%6s %12s\n", "side", "perimeter");
    for (int side = 1; side <= 5; side++) {
        std::vector<point_t> points = square(side);
        std::printf("%6d %12.2f\n", side, geometry_perimeter(points.data(), points.size()));
    }
    return 0;
}
