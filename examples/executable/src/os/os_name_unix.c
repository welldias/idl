#include "os_name.h"

#include <sys/utsname.h>

const char *os_name(void) {
    static struct utsname info;
    if (uname(&info) != 0)
        return "unix";
    return info.sysname;
}
