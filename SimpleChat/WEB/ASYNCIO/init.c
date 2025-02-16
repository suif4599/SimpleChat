#include "asyncio.h"

va_list VA_NULL;

long long currentTimeMillisec(void) {
    #ifdef __linux__
    long long tmp;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    tmp = tv.tv_sec;
    tmp = tmp * 1000;
    tmp = tmp + (tv.tv_usec / 1000);
    return tmp;
    #elif _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER ui;
    ui.LowPart = ft.dwLowDateTime;
    ui.HighPart = ft.dwHighDateTime;
    return (ui.QuadPart - 116444736000000000ULL) / 10000;
    #endif
}