// A C++ executable using the C library: C and C++ can be mixed freely.
#include <cstdio>
#include <string>

#include <shapes/shapes.h>

int main() {
    const std::string line(40, '-');

    std::printf("%8s %15s %15s\n", "radius", "area", "perimeter");
    std::printf("%s\n", line.c_str());
    for (int radius = 1; radius <= 5; radius++)
        std::printf("%8d %15.2f %15.2f\n", radius, shapes_circle_area(radius), shapes_circle_perimeter(radius));
    return 0;
}
