#ifndef __MACRO_H__
#define __MACRO_H__

#define STR_ASSIGN_TO_SCRATCH(dest, src, retval) do { \
    dest = malloc(strlen(src) + 1); \
    if (dest == NULL) { \
        MemoryError("STR_ASSIGN", "Failed to allocate memory for dest"); \
        return retval; \
    } \
    strcpy(dest, src); \
} while (0)

#endif