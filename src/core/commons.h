#ifndef IDL_COMMONS_H
#define IDL_COMMONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) || defined(__CYGWIN__)
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif

    #define  _WINSOCK_DEPRECATED_NO_WARNINGS 

    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>

    #include <share.h>
    #include <io.h>
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <direct.h>

    #include <dirent.h>

    #ifndef __FILE_NAME__
    #define __FILE_NAME__ __FILE__
    #endif

    #define FOLDER_SEPARATOR '\\'
    #define ENV_PATH_SEPARATOR ';'
    #define EXE_EXTENSION ".exe"
    #ifndef access
        #define access _access
    #endif

    #define getcwd _getcwd
    #define mkdir(path, mode) _mkdir(path)
    #define strtok_r strtok_s

    #if defined(_WIN64)
        #define PLATFORM_NAME "Windows 64-bit"
    #else
        #define PLATFORM_NAME "Windows 32-bit"
    #endif
#elif defined(__linux__)
    #ifdef __x86_64__
        #define PLATFORM_NAME "Linux 64-bit"
    #elif __i386__
        #define PLATFORM_NAME "Linux 32-bit"
    #else
        #define PLATFORM_NAME "Linux"
    #endif
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
    #include <errno.h>
    #include <ctype.h>
    #include <dirent.h>
    #include <sys/stat.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <sys/types.h>
    #include <sys/wait.h>

    #define FOLDER_SEPARATOR '/'
    #define ENV_PATH_SEPARATOR ':'
    #define EXE_EXTENSION ""

#elif defined(__APPLE__) && defined(__MACH__)
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR == 1
        #define PLATFORM_NAME "IOS" // Apple iOS
    #elif TARGET_OS_IPHONE == 1
        #define PLATFORM_NAME "IOS" // Apple iOS
    #elif TARGET_OS_MAC == 1
        #define PLATFORM_NAME "OSX" // Apple OSX
    #endif
#elif defined(unix) || defined(__unix__) || defined(__unix)
    #if defined(BSD)
        #define PLATFORM_NAME "BSD" // FreeBSD, NetBSD, OpenBSD, DragonFly BSD
    #endif
#elif defined(__ANDROID__)
    #define PLATFORM_NAME "Android" // Android (implies Linux, so it must come first)
#else
    #error Unknown environment!
#endif

#ifndef SOCKET
#define SOCKET int
#endif

#ifndef INVALID_SOCKET 
#define INVALID_SOCKET -1 
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#include "log.h"

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef size_t word;
typedef uint8_t byte;

#define RETURN_IF_FAIL(expr)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
    if (!(expr)) {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
        log_error("Expression '%s' fail.", #expr);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
        return;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
    }
#define RETURN_VAL_IF_FAIL(expr, val)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
    if (!(expr)) {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
        log_error("Expression '%s' fail. Returing '%s'", #expr, #val);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
        return (val);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
    }

#define LOG_FATAL_NOT_ENOUGH_MEMORY() log_error("There is no memory available for this operation")

#define SIZE_OF_ARRAY(x) ((sizeof(x) / sizeof(0 [x])) / ((word)(!(sizeof(x) % sizeof(0 [x])))))

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#ifndef nullptr
#define nullptr NULL
#endif

/**
 * Callback for collection's item destruction.
 * It will be called for each item when the collection is destroyed, if provided.
 */
typedef void (*common_item_destroy_cb)(void *item);

#endif // IDL_COMMONS_H