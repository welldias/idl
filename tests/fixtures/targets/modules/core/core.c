#include <base/base.h>
#include <core/core.h>

#include "core_private.h"

int core_value(void) {
    return base_value() + CORE_LEVEL + CORE_SECRET;
}
