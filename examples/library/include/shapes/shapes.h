#ifndef SHAPES_H
#define SHAPES_H

/* Public API of the library: include/ is on the include path, so users write
   #include <shapes/shapes.h>. */

#ifdef __cplusplus
extern "C" {
#endif

double shapes_circle_area(double radius);
double shapes_circle_perimeter(double radius);

double shapes_rectangle_area(double width, double height);
double shapes_rectangle_perimeter(double width, double height);

#ifdef __cplusplus
}
#endif

#endif
