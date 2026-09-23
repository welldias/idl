#include <stdio.h>
#include <core/core.h> /* visible through the plugin's link */

int plugin_value(void);

int main(void) {
    printf("plugin=%d version=%d\n", plugin_value(), CORE_VERSION);
    return 0;
}
