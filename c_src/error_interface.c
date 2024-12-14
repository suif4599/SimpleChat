#include <stdio.h>
#include <string.h>
#include "error_interface.h"
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <errno.h>
#endif

void show_error(char* type, char *message)
{
    printf("%s: %s", type, message);
    #ifdef _WIN32
    printf(" Error code: %d", GetLastError());
    #elif __linux__
    printf(" Error code: %d", errno);
    #endif
}