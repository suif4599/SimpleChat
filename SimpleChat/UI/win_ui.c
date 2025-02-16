#include "ui.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <synchapi.h>


static char *buffer = NULL;
static CRITICAL_SECTION cs;
static DWORD WINAPI ThreadProc(LPVOID lpParam) {
	while (1) {
		// read from stdin
		char* buf = malloc(STDIN_BUFFER_SIZE);
		int buf_len = STDIN_BUFFER_SIZE;
		char* ret = fgets(buf, STDIN_BUFFER_SIZE, stdin);
		if (ret == NULL) {
			printf("SystemError, unable to read stdin\n");
			return 1;
		}
		else {
			while (buf[strlen(buf) - 1] != '\n') {
				buf = realloc(buf, buf_len += STDIN_BUFFER_SIZE);
				ret = fgets(buf + buf_len - STDIN_BUFFER_SIZE - 1, STDIN_BUFFER_SIZE, stdin);
				if (ret == NULL) {
					printf("SystemError, unable to read stdin\n");
					return 1;
				}
			}
			EnterCriticalSection(&cs);
			if (buffer != NULL) free(buffer); // free previous buffer (if any
			buffer = malloc(strlen(buf) + 1);
			strcpy(buffer, buf);
			LeaveCriticalSection(&cs);
			free(buf);
		}
		Sleep(50);
	}
	return 0;
}

int InitializeUserInterface() {
	InitializeCriticalSection(&cs);
	HANDLE hThread = CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL);
	if (hThread == NULL) {
		ThreadError("InitializeUserInterface", "Failed to create thread");
		return -1;
	}
	return 0;
}

char *readLine(int n) { // n is not used
	EnterCriticalSection(&cs);
	if (buffer != NULL) {
		buffer[strlen(buffer) - 1] = '\0'; // remove '\n'
		char* new_buffer = malloc(strlen(buffer) + 1);
		if (new_buffer == NULL) {
			MemoryError("readLine", "Failed to allocate memory for new buffer");
			LeaveCriticalSection(&cs);
			return NULL;
		}
		strcpy(new_buffer, buffer);
		free(buffer);
		buffer = NULL;
		LeaveCriticalSection(&cs);
		return new_buffer;
	}
	LeaveCriticalSection(&cs);
	return NULL;
}

int moveUpFirst() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetConsoleScreenBufferInfo(hStdout, &csbi)) {
		SystemError("moveUpFirst", "Failed to get console screen buffer info");
		return -1;
	}
	COORD coord = {
		0,
		csbi.dwCursorPosition.Y - 1
	};
    SetConsoleCursorPosition(hStdout, coord);
	return 0;
}

#endif