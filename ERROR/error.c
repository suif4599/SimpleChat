#include "error.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>


Error* GLOBAL_ERROR = NULL;
static int KEYBOARD_INTERRUPT = 0; // Set to 1 if the program is interrupted by the user
static const char* REPEATED_ERROR_NAME = "RepeatedError";

void print_system_error(const char *prefix) {
#ifdef _WIN32
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError();
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );
    if (prefix != NULL) printf("%s: ", prefix);
    printf("%s\n", lpMsgBuf);
    LocalFree(lpMsgBuf);
#elif __linux__
    if (prefix != NULL) printf("%s: ", prefix);
    char buf[1024];
#if defined(__GLIBC__) || defined(__GNU_LIBRARY__)
    strerror_r(errno, buf, sizeof(buf));
    printf(YELLOW_TERMINAL "%s" RESET_TERMINAL "\n", buf);
#else
    printf("%s\n", strerror_r(errno, buf, sizeof(buf)));
#endif
#endif
}

void ForceExit(const char* name, const char* raiser, const char* message) {
    printf("Memory error occured when %s raises <%s>: %s\n", raiser, name, message);
    PrintError();
    ReleaseError();
    exit(1);
}


void RaiseError(const char* name, const char* raiser, const char* message, const char* file, int line, void* data) {
    Error* error = malloc(sizeof(Error));
    if (error == NULL)
        ForceExit(name, raiser, message);
    error->name = (char*)malloc(strlen(name) + 1);
    if (error->name == NULL)
        ForceExit(name, raiser, message);
    strcpy(error->name, name);
    error->raiser = (char*)malloc(strlen(raiser) + 1);
    if (error->raiser == NULL)
        ForceExit(name, raiser, message);
    strcpy(error->raiser, raiser);
    error->message = (char*)malloc(strlen(message) + 1);
    if (error->message == NULL)
        ForceExit(name, raiser, message);
    strcpy(error->message, message);
    error->file = (char*)malloc(strlen(file) + 1);
    if (error->file == NULL)
        ForceExit(name, raiser, message);
    strcpy(error->file, file);
    error->line = line;
    error->data = data;
    error->next = GLOBAL_ERROR;
    GLOBAL_ERROR = error;
}

void RepeatError(const char* raiser, const char* file, int line) {
    RaiseError(REPEATED_ERROR_NAME, raiser, "Repeated error", file, line, NULL);
}

void ReleaseError() {
    Error* error = GLOBAL_ERROR;
    while (error != NULL) {
        Error* next = error->next;
        free(error->name);
        free(error->raiser);
        free(error->message);
        if (error->data != NULL)
            free(error->data);
        free(error);
        error = next;
    }
}

void PrintError() {
    #ifdef __linux__
    printf(RED_TERMINAL "User Traceback (most recent call last):" RESET_TERMINAL "\n");
    #endif
    Error* error = GLOBAL_ERROR;
    while (error != NULL) {
        #ifdef __linux__
        printf(YELLOW_TERMINAL "%s" RESET_TERMINAL " from " BOLD_TERMINAL "%s" RESET_TERMINAL " in " BOLD_TERMINAL "%s" RESET_TERMINAL ", line %d\n", 
            error->name, error->raiser, error->file, error->line);
        #endif
        printf("    %s\n", error->message);
        error = error->next;
    }
    printf(RED_TERMINAL "System error info:" RESET_TERMINAL "\n");
    print_system_error(NULL);
}

Error* CatchError(const char* name) {
    Error* error = GLOBAL_ERROR;
    while (error != NULL) {
        if (strcmp(error->name, name) == 0)
            return error;
        error = error->next;
    }
    return NULL;
}

void sigint_handler(int signo) {
    KEYBOARD_INTERRUPT = 1;
}

int InitErrorStream() {
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        RaiseError("SignalError", "InitErrorStream", "Failed to set signal handler for SIGINT", __FILE__, __LINE__, NULL);
        return -1;
    }
    return 0;
}

int RefreshErrorStream() {
    if (KEYBOARD_INTERRUPT) {
        RaiseError("KeyboardInterrupt", "RefreshErrorStream", "Keyboard interrupt", __FILE__, __LINE__, NULL);
        return -1;
    }
    return 0;
}