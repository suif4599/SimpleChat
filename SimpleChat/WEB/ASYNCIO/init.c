#include "asyncio.h"

va_list VA_NULL;

long long currentTimeMillisec(void) {
    long long tmp;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    tmp = tv.tv_sec;
    tmp = tmp * 1000;
    tmp = tmp + (tv.tv_usec / 1000);
    return tmp;
}
