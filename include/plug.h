#ifndef PLUG_H_
#define PLUG_H_


#define LIST_OF_PLUGS(PLUG_GENERATOR) \
    PLUG_GENERATOR(init,          void, void)         /* Initialize the plugin */ \
    PLUG_GENERATOR(save_state,    void*, void)        /* Prepare old plugin, return it's state */ \
    PLUG_GENERATOR(load_state,    void, void*)        /* Give plugin previous state */ \
    PLUG_GENERATOR(free_state,    void, void)         /* Free any memory that needs to be freed */ \
    PLUG_GENERATOR(update,        void, void*)        /* Update state with given context data */ \
    PLUG_GENERATOR(reset,         void, void)         /* Reset the state of the plugin */ \


// Use to generate function pointers
#define PLUG_FUNC_PTR(name, ret, ...) ret (*name)(__VA_ARGS__);

typedef struct PlugLib {
    void *lib_ptr;
    LIST_OF_PLUGS(PLUG_FUNC_PTR)
} PlugLib;

/*

// Create function pointers in a host object


// Load function pointers after loading library using load_library.x
#define PLUG_LOAD(func_name, ...) \
    func_name = LibGetSymbol(lib_ptr, #func_name);  \
    if (func_name == NULL){                         \
        return;                                     \
    }                                               \
LIST_OF_PLUGS(PLUG_LOAD)
#undef PLUG_LOAD
*/


#endif // PLUG_H_