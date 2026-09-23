#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <shapes/shapes.h>

static int usage(void) {
    fprintf(stderr, "usage: area circle <radius>\n"
                    "       area rectangle <width> <height>\n");
    return 1;
}

int main(int argc, char **argv) {
    if (argc == 3 && strcmp(argv[1], "circle") == 0) {
        double radius = atof(argv[2]);
        printf("area=%.2f perimeter=%.2f\n", shapes_circle_area(radius), shapes_circle_perimeter(radius));
        return 0;
    }

    if (argc == 4 && strcmp(argv[1], "rectangle") == 0) {
        double width = atof(argv[2]);
        double height = atof(argv[3]);
        printf("area=%.2f perimeter=%.2f\n", shapes_rectangle_area(width, height), shapes_rectangle_perimeter(width, height));
        return 0;
    }

    return usage();
}
