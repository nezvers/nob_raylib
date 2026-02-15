#include <stdio.h>
#include "plug_host.h"
#include "load_library.h"

bool LoadPlug(const char *plug_path, PlugApi *plug_api) {
    plug_api->lib_handle = LibLoad(plug_path);
    if (!LibIsValid(plug_api->lib_handle)) {
        return false;
    }

#define FOR_EACH_FUNC(name, return_type, ...)       \
    plug_api->name = LibGetSymbol(#name);           \
    if (plug_api->name == NULL) {                   \
        fprintf(stderr, "ERROR: %s\n", LibError()); \
        UnloadPlug(plug_api->lib_handle);           \
        return false;                               \
    }                                               \
    LIST_OF_FUNCTIONS(FOR_EACH_FUNC)
#undef FOR_EACH_FUNC

    return true;
}

void UnloadPlug(PlugApi *plug_api) {
    if (plug_api == NULL) {
        return;
    }
    if (plug_api->lib_handle == NULL) {
        return;
    }
    if (!LibIsValid(plug_api->lib_handle)) {
        return;
    }
    plug_api->free_state(plug_api->plug_state);
#define FOR_EACH_FUNC(name, return_type, ...)   \
    plug_api->name = NULL;                      \
    LIST_OF_FUNCTIONS(FOR_EACH_FUNC)
#undef FOR_EACH_FUNC
    LibUnload(plug_api->lib_handle);
}