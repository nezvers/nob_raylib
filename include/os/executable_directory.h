#ifndef EXEC_DIR_H
#define EXEC_DIR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
    Writes the directory of the current executable into `buffer`.

    Parameters:
        buffer      - destination buffer
        buffer_size - size of buffer in bytes

    Returns:
        0   success
       -1   failure
       -2   buffer too small

    The resulting string:
        - is null terminated on success
        - does NOT include trailing slash
*/
int GetExecutableDirectory(char *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* EXEC_DIR_H */
