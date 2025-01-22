#ifndef __ERROR_H__
#define __ERROR_H__


typedef struct {
    char* name;
    char* raiser;
    char* message;
    void* data; // May be NULL
    Error* next;
} Error;

extern Error* GLOBAL_ERROR;

void RaiseError(const char* name, char* raiser, const char* message, void* data);
void RepeatError(const char* raiser); // Repeat the last error, used if the function doesn't handle the error
void ReleaseError();
void PrintError(); // Print all errors in the global error list, it WON'T release the error list
Error* CatchError(const char* name); // Return the first error with the given name or NULL if not found, it WON'T release the error list

#define MemoryError(raiser, message) RaiseError("MemoryError", raiser, message, NULL)
#define RepeatedError(raiser) RepeatError(raiser) // ("RepeatedError", raiser, message, NULL)

#endif