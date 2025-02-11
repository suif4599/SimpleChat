#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SimpleChat/WEB/ASYNCIO/asyncio.h"
#include "ERROR/error.h"
#include "MACRO/macro.h"

ASYNC_DEF(say, int, delay, char*, s)
    AWAIT(COROUTINE(AsyncSleep, delay));
    printf("    Say after %dms: %s\n", delay, s);
ASYNC_END_DEF

ASYNC_DEF(g, char*, s)
    printf("    Begin: %s\n", s);
    AWAIT(
        COROUTINE(say, 1000, s),
        COROUTINE(say, 2000, s)
    )
    printf("    End: %s\n", s);
ASYNC_END_DEF

int __main() {
    EventLoop* evlp = CreateEventLoop();
    if (evlp == NULL) goto ERROR_END;
    if (RegisterAsyncFunction(evlp, say) == -1) goto ERROR_END;
    if (RegisterAsyncFunction(evlp, g) == -1) goto ERROR_END;
    if (ASYNC_CALL(evlp, g, "hello") == -1) goto ERROR_END;
    if (EventLoopRun(evlp, 10) == -1) goto ERROR_END;
    return 0;
ERROR_END:
    RepeatedError("main");
    if (ReleaseEventLoop(evlp) == -1) {
        RepeatedError("main.release");
    }
    return -1;
}

int main() {
    if (__main() == -1) {
        PrintError();
        return -1;
    }
    return 0;
}