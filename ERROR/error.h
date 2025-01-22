#ifndef __ERROR_H__
#define __ERROR_H__

#ifdef _WIN32
    #include <windows.h>
#else
    #include <errno.h>
#endif

// 获取当前错误码的函数
#ifdef _WIN32
#define GET_LAST_ERROR GetLastError()
#else
#define GET_LAST_ERROR errno
#endif


typedef struct ERROR_TYPE{
    char* name;
    char* raiser;
    char* message;
    void* data; // May be NULL
    int _errno; // Set to -1 if not used 
    struct ERROR_TYPE* next;
} Error;

extern Error* GLOBAL_ERROR;

void RaiseError(const char* name, const char* raiser, const char* message, void* data);
void RepeatError(const char* raiser); // Repeat the last error, used if the function doesn't handle the error
void ReleaseError();
void PrintError(); // Print all errors in the global error list, it WON'T release the error list
Error* CatchError(const char* name); // Return the first error with the given name or NULL if not found, it WON'T release the error list

#define MemoryError(raiser, message) RaiseError("MemoryError", raiser, message, NULL)
#define RepeatedError(raiser) RepeatError(raiser) // ("RepeatedError", raiser, message, NULL)
#define SocketError(raiser, message) RaiseError("SocketError", raiser, message, NULL)

#endif