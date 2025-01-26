#include "ui.h"

#ifdef __linux__
char *readLine(int n) {
    size_t len = 0;
    int bufferSize = n;
    char *buffer = (char *)malloc(bufferSize);
    if (buffer == NULL) {
        MemoryError("readLine", "Failed to allocate memory for buffer");
        return NULL;
    }
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
        if (len + 1 >= bufferSize) {
            bufferSize += n;
            buffer = (char *)realloc(buffer, bufferSize);
            if (buffer == NULL) {
                MemoryError("readLine", "Failed to reallocate memory for buffer");
                free(buffer);
                return NULL;
            }
        }
        buffer[len++] = (char)c;
    }
    buffer[len] = '\0';
    return buffer;
}
#endif