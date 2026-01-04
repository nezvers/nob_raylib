#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "plug.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PLUG_DEFINITION(name, ret, ...) ret name(__VA_ARGS__);
LIST_OF_PLUGS(PLUG_DEFINITION)
#undef PLUG_DEFINITION

#ifdef __cplusplus
}
#endif

typedef struct {
    size_t size;
} PlugState;

static PlugState *p = NULL;

static void load_assets(void){
    printf("load_assets()\n");
}

static void unload_assets(void){
    printf("unload_assets()\n");
}

void init(void){
    p = malloc(sizeof(*p));
    assert(p != NULL);
    memset(p, 0, sizeof(*p));
    p->size = sizeof(*p);

    load_assets();
    plug_reset();
}

void *save_state(void){
    printf("save_state()\n");
    unload_assets();
    return p;
}

void load_state(void *state){
    p = state;
    if (p->size < sizeof(*p)) {
        //TraceLog(LOG_INFO, "Migrating plug state schema %zu bytes -> %zu bytes", p->size, sizeof(*p));
        p = realloc(p, sizeof(*p));
        p->size = sizeof(*p);
    }

    load_assets();
}

void free_state(void){
    printf("free_state()\n");
    unload_assets();
}

void update(void *data){
    printf("update()\n");
}

void reset(void){
    printf("reset()\n");
}
