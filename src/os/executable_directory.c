#include "os/executable_directory.h"

#include <string.h>

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int GetExecutableDirectory(char *buffer, size_t buffer_size)
{
    DWORD len;

    if (!buffer || buffer_size == 0)
        return -1;

    len = GetModuleFileNameA(NULL, buffer, (DWORD)buffer_size);
    if (len == 0 || len == buffer_size)
        return -1;

    /* Strip filename */
    for (DWORD i = len; i > 0; --i) {
        if (buffer[i - 1] == '\\' || buffer[i - 1] == '/') {
            buffer[i - 1] = '\0';
            return 0;
        }
    }

    return -1;
}

#elif defined(__APPLE__)

#include <mach-o/dyld.h>
#include <limits.h>
#include <unistd.h>

int GetExecutableDirectory(char *buffer, size_t buffer_size)
{
    uint32_t size = (uint32_t)buffer_size;

    if (!buffer || buffer_size == 0)
        return -1;

    if (_NSGetExecutablePath(buffer, &size) != 0)
        return -2; /* buffer too small */

    /* Resolve symlinks */
    char resolved[PATH_MAX];
    if (!realpath(buffer, resolved))
        return -1;

    if (strlen(resolved) >= buffer_size)
        return -2;

    strcpy(buffer, resolved);

    /* Strip filename */
    for (size_t i = strlen(buffer); i > 0; --i) {
        if (buffer[i - 1] == '/') {
            buffer[i - 1] = '\0';
            return 0;
        }
    }

    return -1;
}

#elif defined(__linux__)

#include <unistd.h>
#include <limits.h>

int GetExecutableDirectory(char *buffer, size_t buffer_size)
{
    ssize_t len;

    if (!buffer || buffer_size == 0)
        return -1;

    len = readlink("/proc/self/exe", buffer, buffer_size - 1);
    if (len == -1)
        return -1;

    if ((size_t)len >= buffer_size)
        return -2;

    buffer[len] = '\0';

    /* Strip filename */
    for (ssize_t i = len; i > 0; --i) {
        if (buffer[i - 1] == '/') {
            buffer[i - 1] = '\0';
            return 0;
        }
    }

    return -1;
}

#else

int GetExecutableDirectory(char *buffer, size_t buffer_size)
{
    (void)buffer;
    (void)buffer_size;
    return -1; /* Unsupported platform */
}

#endif
