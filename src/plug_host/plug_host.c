#include <stdio.h>
#include "plug_host.h"
#include "load_library.h"

#define LOAD_PLUG_FUNCTIONS(name, return_type, ...)             \
    plug_api->name = LibGetSymbol(plug_api->lib_handle, #name); \
    if (plug_api->name == NULL) {                               \
        fprintf(stderr, "ERROR: %s\n", LibError());             \
        PlugUnload(plug_api->lib_handle);                       \
        return false;                                           \
    }                                                           \

#define UNLOAD_PLUG_FUNCTIONS(name, return_type, ...)   \
    plug_api->name = NULL;                      \


bool PlugLoad(const char *plug_path, PlugApi *plug_api) {
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