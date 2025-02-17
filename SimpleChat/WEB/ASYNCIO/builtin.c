#include "asyncio.h"
#include <stdio.h>

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
            MemoryError("AsyncSleep", "Failed to allocate memory for event");
            goto ERROR_END;
        }
        event->target_time = (long long)va_arg(args_copy, int) + currentTimeMillisec();
        if (LinkAppend(&evlp->sleep_events, event) == -1) {
            RepeatedError("AsyncSleep");
            free(event);
            goto ERROR_END;
        }
        if (add_depend) {
            event->caller_frame = evlp->active_frame;
        } else {
            event->caller_frame = NULL;
            Warn("AsyncSleep shouldn't be detached");
            goto NORMAL_END;
        }
        if (LinkAppend(&evlp->active_frame->dependency, event) == -1) {
            RepeatedError("AsyncSleep");
            free(event);
            goto ERROR_END;
        }
        goto NORMAL_END;
    }
    if (strcmp(func_name, "AsyncAccept") == 0) {
        // listen socket is normalled unbound
        AsyncSocket* async_socket = va_arg(args_copy, AsyncSocket*);
        // printf("[CheckBuiltinAndCall]: Accept: %s:%d\n", async_socket->ip, async_socket->port);
        async_socket->result_temp = (void**)va_arg(args_copy, AsyncSocket**);
        if (async_socket->caller_frame != NULL) {
            AsyncError("AsyncAccept", "The socket is already in use");
            goto ERROR_END;
        }
        if (!async_socket->is_listen_socket) {
            AsyncError("AsyncAccept", "The socket is not a listen socket");
            goto ERROR_END;
        }
        if (add_depend) {
            if (evlp->active_frame == NULL) {
                AsyncError("AsyncAccept", "The frame is not active");
                goto ERROR_END;
            }
            async_socket->caller_frame = evlp->active_frame;
            if (LinkAppend(&async_socket->caller_frame->dependency, async_socket) < 0) {
                RepeatedError("AsyncAccept");
                goto ERROR_END;
            }
        } else {
            async_socket->caller_frame = NULL;
        }
        if (BindAsyncSocket(evlp, async_socket) == -1) {
            LinkDeleteData(&evlp->active_frame->dependency, async_socket);
            RepeatedError("AsyncAccept");
            goto ERROR_END;
        }
        goto NORMAL_END;
    }
    if (strcmp(func_name, "AsyncRecv") == 0) {
        // receive socket is normalled bound
        AsyncSocket* async_socket = va_arg(args_copy, AsyncSocket*);
        async_socket->result_temp = (void**)va_arg(args_copy, char**);
        if (async_socket->caller_frame != NULL) {
            AsyncError("AsyncRecv", "The socket is already in use");
            goto ERROR_END;
        }
        if (!async_socket->is_receive_socket || async_socket->is_listen_socket) {
            AsyncError("AsyncRecv", "The socket is not a receive socket");
            goto ERROR_END;
        }
        if (add_depend) {
            if (evlp->active_frame == NULL) {
                AsyncError("AsyncRecv", "The frame is not active");
                goto ERROR_END;
            }
            async_socket->caller_frame = evlp->active_frame;
            if (LinkAppend(&evlp->active_frame->dependency, async_socket) < 0) {
                RepeatedError("AsyncRecv");
                goto ERROR_END;
            }
        } else {
            async_socket->caller_frame = NULL;
        }
        if (LinkAppend(&evlp->recv_sockets, async_socket) == -1) {
            RepeatedError("AsyncRecv");
            goto DELETE_DEPENDENCY;
        }
        goto NORMAL_END;
    DELETE_DEPENDENCY:
        LinkDeleteData(&evlp->active_frame->dependency, async_socket);
        goto ERROR_END;
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