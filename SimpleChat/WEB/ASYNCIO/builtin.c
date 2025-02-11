#include "asyncio.h"


int CheckBuiltinAndCall(const char* func_name, int add_depend, va_list args) {
    // args: 0, evlp and other arguments
    // return -1 if error, 0 if found, 1 if not found
    va_list args_copy;
    va_copy(args_copy, args);
    va_arg(args_copy, int);
    EventLoop* evlp = va_arg(args_copy, EventLoop*);
    if (strcmp(func_name, "AsyncSleep") == 0) {
        AsyncSleepEvent* event = (AsyncSleepEvent*)malloc(sizeof(AsyncSleepEvent));
        if (event == NULL) {
            MemoryError("CheckBuiltinAndCall", "Failed to allocate memory for event");
            goto ERROR_END;
        }
        event->target_time = (long long)va_arg(args_copy, int) + currentTimeMillisec();
        event->caller_frame = evlp->active_frame;
        if (LinkAppend(&evlp->sleep_events, event) == -1) {
            RepeatedError("CheckBuiltinAndCall");
            free(event);
            goto ERROR_END;
        }
        if (LinkAppend(&evlp->active_frame->dependency, event) == -1) {
            RepeatedError("CheckBuiltinAndCall");
            free(event);
            goto ERROR_END;
        }
        goto NORMAL_END;
    }
    va_end(args_copy);
    return 1;
ERROR_END:
    va_end(args_copy);
    return -1;
NORMAL_END:
    va_end(args_copy);
    return 0;
}