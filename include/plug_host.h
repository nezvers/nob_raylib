#ifndef PLUG_HOST_H
#define PLUG_HOST_H

#include <stdbool.h>
#include "plug.h"

// Use to generate function exports
#define EXPORT_PLUG_FUNC(name, return_type, ...) return_type (*name)(__VA_ARGS__);

typedef struct {
    void *lib_handle;
    void *plug_state;
LIST_OF_FUNCTIONS(EXPORT_PLUG_FUNC)
} PlugApi;
#undef EXPORT_PLUG_FUNC

bool LoadPlug(const char *plug_path, PlugApi *plug_api);
void UnloadPlug(PlugApi *plug_api);

#endif // PLUG_HOST_H