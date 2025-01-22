#include "error.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


extern Error* GLOBAL_ERROR = NULL;
static const char* REPEATED_ERROR_NAME = "RepeatedError";
static const char* REPEATED_ERROR_MSG = "NULL";

void ForceExit(const char* name, char* raiser, const char* message) {
    printf("Memory error occured when %s raises <%s>: %s\n", raiser, name, message);
    PrintError();
    ReleaseError();
    exit(1);
}


void RaiseError(const char* name, char* raiser, const char* message, void* data) {
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
    error->data = data;
    error->next = GLOBAL_ERROR;
}

void RepeatError(const char* raiser) {
    RaiseError(REPEATED_ERROR_NAME, raiser, REPEATED_ERROR_MSG, NULL);
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
    printf("Traceback (most recent call last):\n");
    Error* error = GLOBAL_ERROR;
    while (error != NULL) {
        if (strcmp(error->name, REPEATED_ERROR_NAME) == 0) {
            printf("    Repeated from %s\n", error->raiser);
        } else {
            printf("    Error: %s\n", error->name);
            printf("        Raiser: %s\n", error->raiser);
            printf("        Message: %s\n", error->message);
        }
        error = error->next;
    }
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