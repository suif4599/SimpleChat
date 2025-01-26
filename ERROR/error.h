#ifndef __ERROR_H__
#define __ERROR_H__

#ifdef _WIN32
    #include <windows.h>
#else
    #include <errno.h>
#endif

// get the last error code
#ifdef _WIN32
#define GET_LAST_ERROR GetLastError()
#else
#define GET_LAST_ERROR errno
#endif



typedef struct ERROR_TYPE{
    char* name;
    char* raiser;
    char* message;
    char* file;
    int line;
    void* data; // May be NULL
    int _errno; // Set to -1 if not used 
    struct ERROR_TYPE* next;
} Error;

extern Error* GLOBAL_ERROR;

int InitErrorStream(); // Initialize the error stream, return 0 if success, otherwise return -1
int RefreshErrorStream(); // Refresh the error stream, it should be called in the mainloop, return 0 if success, otherwise return -1
void RaiseError(const char* name, const char* raiser, const char* message, const char* file, int line, void* data); // Raise a new error, it will be added to the global error list
void RepeatError(const char* raiser, const char* file, int line); // Repeat the last error, used if the function doesn't handle the error
void ReleaseError();
void PrintError(); // Print all errors in the global error list, it WON'T release the error list
Error* CatchError(const char* name); // Return the first error with the given name or NULL if not found, it WON'T release the error list

#define MemoryError(raiser, message) RaiseError("MemoryError", raiser, message, __FILE__, __LINE__, NULL)
#define RepeatedError(raiser) RepeatError(raiser, __FILE__, __LINE__) // ("RepeatedError", raiser, message, NULL)
#define SocketError(raiser, message) RaiseError("SocketError", raiser, message, __FILE__, __LINE__, NULL)
#define EpollError(raiser, message) RaiseError("EpollError", raiser, message, __FILE__, __LINE__, NULL)
#define SystemError(raiser, message) RaiseError("SystemError", raiser, message, __FILE__, __LINE__, NULL)
#define EventError(raiser, message) RaiseError("EventError", raiser, message, __FILE__, __LINE__, NULL)
#define ThreadError(raiser, message) RaiseError("ThreadError", raiser, message, __FILE__, __LINE__, NULL)

#ifdef __linux__
#define RED_TERMINAL "\033[31m"
#define YELLOW_TERMINAL "\033[33m"
#define BOLD_TERMINAL "\033[1m"
#define RESET_TERMINAL "\033[0m"
#endif

#endif