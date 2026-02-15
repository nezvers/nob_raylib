#ifndef PLUG_H_
#define PLUG_H_

/* Detect Windows */
#if defined(_WIN32) || defined(__CYGWIN__)
  #ifdef PLUG_STATIC
    #define PLUG_API
  #else
    #ifdef PLUG_BUILD
      #define PLUG_API __declspec(dllexport)
    #else
      #define PLUG_API __declspec(dllimport)
    #endif
  #endif
#else
  /* Linux / macOS */
  #if __GNUC__ >= 4
    #define PLUG_API __attribute__((visibility("default")))
  #else
    #define PLUG_API
  #endif
#endif

#ifdef __cplusplus
  #define PLUG_EXTERN extern "C"
#else
  #define PLUG_EXTERN extern
#endif

#define LIST_OF_FUNCTIONS(FUNC_GENERATOR) \
    FUNC_GENERATOR(init,          void, void)         /* Initialize the plugin */ \
    FUNC_GENERATOR(save_state,    void*, void)        /* Prepare old plugin, return it's state */ \
    FUNC_GENERATOR(load_state,    void, void*)        /* Give plugin previous state */ \
    FUNC_GENERATOR(free_state,    void, void*)         /* Free any memory that needs to be freed */ \
    FUNC_GENERATOR(update,        void, void*)        /* Update state with given context data */ \
    FUNC_GENERATOR(reset,         void, void)         /* Reset the state of the plugin */ \

// Use to generate function exports
#define EXPORT_PLUG_FUNC(name, return_type, ...) PLUG_EXTERN PLUG_API return_type name(__VA_ARGS__);
LIST_OF_FUNCTIONS(EXPORT_PLUG_FUNC)
#undef EXPORT_PLUG_FUNC

#undef PLUG_API
#undef PLUG_EXTERN

#endif // PLUG_H_