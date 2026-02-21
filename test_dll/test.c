
#define MYLIB_BUILD // Export functions for DLL unless MYLIB_STATIC is defined
#include "test.h"
#include <stdio.h>

void print_hello() {
    printf("Hello from DLL!!!\n");
}