#include <stdio.h>
#include <string.h>

#include <geometry/geometry.h>

int main(void) {
    point_t one[] = { { 1, 1 } };
    point_t two[] = { { 0, 0 }, { 1, 0 } };
    char text[64];
    int ok = 1;

    geometry_describe(text, sizeof(text), one, 1);
    ok = ok && strcmp(text, "1 point, perimeter 0.00") == 0;
    geometry_describe(text, sizeof(text), two, 2);
    ok = ok && strcmp(text, "2 points, perimeter 2.00") == 0;

    printf("describe: %s\n", ok ? "ok" : "FAILED");
    return ok ? 0 : 1;
}
