#include <stdio.h>

int plugin_value(void); /* from the shared library, found through the rpath of tests/ */

int main(void) {
    int ok = plugin_value() == 38;
    printf("plugin: %s\n", ok ? "ok" : "wrong");
    return ok ? 0 : 1;
}
