#ifndef LOAD_LIBRARY_H
#define LOAD_LIBRARY_H

#include <stdbool.h>

// Get library. Give relative file name (win:*.dll, lin: *.so )
void* LibLoad(const char *lib_name);

// Get pointer to available symbol inside library (function, variable)
void* LibGetSymbol(void *lib_ptr, const char *symbol_name);

// Check if returned pointer is valid
bool LibIsValid(void *lib_ptr);

// Unload library
void LibUnload(void *lib_ptr);

// Get last error
const char* LibError(void);


#endif // LOAD_LIBRARY_H