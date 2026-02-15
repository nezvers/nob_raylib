#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PLUG_BUILD // Exports functions for DLL
#include "plug.h"


typedef struct {
    size_t size;
} PlugState;

static PlugState *plug_state = NULL;

static void load_assets(void){
    printf("load_assets()\n");
}

static void unload_assets(void){
    printf("unload_assets()\n");
}

/* exported */
void reset(void){
    printf("reset()\n");
}

/* exported */
void init(void){
    plug_state = malloc(sizeof(*plug_state));
    assert(plug_state != NULL);
    memset(plug_state, 0, sizeof(*plug_state));
    plug_state->size = sizeof(*plug_state);

    printf("init()\n");
    load_assets();
    reset();
}

/* exported */
void *save_state(void){
    printf("save_state()\n");
    return plug_state;
}

/* exported */
void load_state(void *state){
    plug_state = state;
    if (plug_state->size < sizeof(*plug_state)) {
        //TraceLog(LOG_INFO, "Migrating plug state schema %zu bytes -> %zu bytes", p->size, sizeof(*p));
        plug_state = realloc(plug_state, sizeof(*plug_state));
        plug_state->size = sizeof(*plug_state);
    }

    load_assets();
}

/* exported */
void free_state(void){
    printf("free_state()\n");
    unload_assets();
    
    free(plug_state);
}

/* exported */
void update(void *data){
    (void)data;
    printf("update()\n");
}
