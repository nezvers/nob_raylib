#include "load_library.h" // only signatures of these functions
#include <stdbool.h>
#include <stdio.h>

#ifdef _WIN32
// #    define _WIN32_WINNT 0x0501 // win XP - https://learn.microsoft.com/en-us/cpp/porting/modifying-winver-and-win32-winnt?view=msvc-170
#    include <windows.h> // requires linking: with freestanding build Kernel32.lib / Kernel32.dll
#    include <winerror.h>
#else
#    include <dlfcn.h>
#endif

// Get library. Give relative file name (win:*.dll, lin: *.so )
void* LibLoad(const char *lib_name){
#ifdef _WIN32
    return (void *)LoadLibraryA(lib_name);
#else
    // RTLD_GLOBAL is recommended if the library exports symbols needed by other libs
    return dlopen(lib_name, RTLD_NOW | RTLD_GLOBAL);
#endif
}

// Get pointer to available symbol inside library (function, variable)
void* LibGetSymbol(void *lib_ptr, const char *symbol_name){
    if (lib_ptr == NULL) return NULL;
#ifdef _WIN32
    return (void *)GetProcAddress((HMODULE)lib_ptr, symbol_name);
#else
    return dlsym(lib_ptr, symbol_name);
#endif
}

// Check if returned pointer is valid
bool LibIsValid(void *lib_ptr){
    return lib_ptr != NULL;
}

// Unload library
void LibUnload(void *lib_ptr){
    if (lib_ptr == NULL) return;
#ifdef _WIN32
    FreeLibrary((HMODULE)lib_ptr);
#else
    dlclose(lib_ptr);
#endif
}

// Get last error
const char* LibError(void){
#ifdef _WIN32
    static char buf[512];
    DWORD err = GetLastError();
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   buf, sizeof(buf), NULL);
    return buf;
#else
    return dlerror();   // note: dlerror() returns a pointer to static storage
#endif
}
