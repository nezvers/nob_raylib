#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PLUG_BUILD // Exports functions for DLL
#include "plug.h"


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

/* exported */
void reset(void){
    printf("reset()\n");
}

/* exported */
void init(void){
    p = malloc(sizeof(*p));
    assert(p != NULL);
    memset(p, 0, sizeof(*p));
    p->size = sizeof(*p);

    load_assets();
    reset();
}

/* exported */
void *save_state(void){
    printf("save_state()\n");
    unload_assets();
    return p;
}

/* exported */
void load_state(void *state){
    p = state;
    if (p->size < sizeof(*p)) {
        //TraceLog(LOG_INFO, "Migrating plug state schema %zu bytes -> %zu bytes", p->size, sizeof(*p));
        p = realloc(p, sizeof(*p));
        p->size = sizeof(*p);
    }

    load_assets();
}

/* exported */
void free_state(void *state){
    printf("free_state()\n");
    unload_assets();
    
    free(state);
}

/* exported */
void update(void *data){
    (void)data;
    printf("update()\n");
}
