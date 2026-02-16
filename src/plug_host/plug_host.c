#include <stdio.h>
#include "plug_host.h"
#include "load_library.h"

/* Generate Plug functions in PlugLoad()*/
#define LOAD_PLUG_FUNCTIONS(name, return_type, ...)             \
    plug_api->name = LibGetSymbol(plug_api->lib_handle, #name); \
    if (plug_api->name == NULL) {                               \
        fprintf(stderr, "ERROR: %s\n", LibError());             \
        PlugUnload(plug_api->lib_handle);                       \
        return false;                                           \
    }                                                           \

/* Mark function pointers to NULL using generator macro */
#define UNLOAD_PLUG_FUNCTIONS(name, return_type, ...)   \
    plug_api->name = NULL;                              \


bool PlugLoad(const char *plug_path, PlugApi *plug_api) {
    // workaround to get relative *.so path
#if !defined(__win32)
    // Forward declaration of the function. Didn't want to polute the top of the script.
    int PlugGetExecutableDirectory(char *buffer, size_t buffer_size);

    char path[1024];
    if (GetExecutableDirectory(path, sizeof(path)) == 0) {
        plug_path = path;
    }
    // else, hope user gave absolute path
#endif

    plug_api->lib_handle = LibLoad(plug_path);
    if (!LibIsValid(plug_api->lib_handle)) {
        return false;
    }

    PLUG_LIST_FUNCTIONS(LOAD_PLUG_FUNCTIONS)
    return true;
}

void PlugUnload(PlugApi *plug_api) {
    if (plug_api == NULL) {
        return;
    }
    if (plug_api->lib_handle == NULL) {
        return;
    }
    if (!LibIsValid(plug_api->lib_handle)) {
        return;
    }
    plug_api->free_state();

    PLUG_LIST_FUNCTIONS(UNLOAD_PLUG_FUNCTIONS)
    LibUnload(plug_api->lib_handle);
}


/* taken from os/executable_directory.c to work around unix *.so loading relative to executable */
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#include <unistd.h>

int PlugGetExecutableDirectory(char *buffer, size_t buffer_size) {
    uint32_t size = (uint32_t)buffer_size;

    if (!buffer || buffer_size == 0) {
        return -1;
    }

    if (_NSGetExecutablePath(buffer, &size) != 0) {
        return -2; /* buffer too small */
    }

    /* Resolve symlinks */
    char resolved[PATH_MAX];
    if (!realpath(buffer, resolved)) {
        return -1;
    }

    if (strlen(resolved) >= buffer_size) {
        return -2;
    }

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

int PlugGetExecutableDirectory(char *buffer, size_t buffer_size) {
    ssize_t len;

    if (!buffer || buffer_size == 0) {
        return -1;
    }

    len = readlink("/proc/self/exe", buffer, buffer_size - 1);
    if (len == -1) {
        return -1;
    }

    if ((size_t)len >= buffer_size) {
        return -2;
    }

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
#endif
