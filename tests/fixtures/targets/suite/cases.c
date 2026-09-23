#include <core/core.h>

int case_value(void) {
    return core_value() == 19;
}

int case_version(void) {
    return CORE_VERSION == 3;
}
