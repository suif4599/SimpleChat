#include "asyncio.h"
#include <stdio.h>

#ifdef _WIN32
#define SEND_FLAG 0
#elif __linux__
#define SEND_FLAG MSG_NOSIGNAL
#endif

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
        if (async_socket->caller_frame != NULL) {
            AsyncError("AsyncRecv", "The socket is already in use");
            goto ERROR_END;
        }
        if (!async_socket->is_receive_socket || async_socket->is_listen_socket) {
            if (async_socket->is_listen_socket) {
                AsyncError("AsyncRecv", "The socket is a listen socket");
            } else {
                AsyncError("AsyncRecv", "The socket is a send socket");
            }
            goto ERROR_END;
        }
        async_socket->result_temp = (void**)va_arg(args_copy, char**);
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
            goto DELETE_DEPENDENCY_RECV;
        }
        goto NORMAL_END;
    DELETE_DEPENDENCY_RECV:
        LinkDeleteData(&evlp->active_frame->dependency, async_socket);
        goto ERROR_END;
    }
    if (strcmp(func_name, "AsyncSend") == 0) {
        AsyncSocket* async_socket = va_arg(args_copy, AsyncSocket*);
        if (!async_socket->is_bound) {
            if (BindAsyncSocket(evlp, async_socket) < 0) {
                RepeatedError("AsyncSend");
                goto SEND_DISCONNECT;
            }
        }
        if (async_socket->buffer != NULL) {
            RuntimeError("AsyncSend", "The buffer is not empty");
            goto ERROR_END;
        }
        char* msg = va_arg(args_copy, char*);
        if (async_socket->caller_frame != NULL) {
            AsyncError("AsyncSend", "The socket is already in use");
            goto ERROR_END;
        }
        if (async_socket->is_listen_socket || async_socket->is_receive_socket) {
            if (async_socket->is_listen_socket) {
                AsyncError("AsyncSend", "The socket is a listen socket");
            } else {
                AsyncError("AsyncSend", "The socket is not a receive socket");
            }
            goto ERROR_END;
        }
        async_socket->result_temp = (void*)va_arg(args_copy, int*);
        int len = (int)strlen(msg);
        int ret = send(async_socket->socket, msg, len, SEND_FLAG);
        if (ret < 0) {
            int error = GET_LAST_ERROR;
            if (error == 
                #ifdef _WIN32
                WSAEWOULDBLOCK
                #elif __linux__
                EWOULDBLOCK
                #endif
                ) {
                ret = 0;
            } else {
                SocketError("AsyncSend", "Failed to send message");
                goto SEND_DISCONNECT;
            }
        }
        if (ret < len) {
            // need FD_WRITE/EPOLLOUT
            async_socket->buffer = (char*)malloc(len - ret + 1);
            if (async_socket->buffer == NULL) {
                MemoryError("AsyncSend", "Failed to allocate memory for buffer");
                goto ERROR_END;
            }
            strcpy(async_socket->buffer, msg + ret);
            if (add_depend) {
                if (evlp->active_frame == NULL) {
                    AsyncError("AsyncSend", "The frame is not active");
                    goto ERROR_END;
                }
                async_socket->caller_frame = evlp->active_frame;
                if (LinkAppend(&evlp->active_frame->dependency, async_socket) < 0) {
                    RepeatedError("AsyncSend");
                    goto ERROR_END;
                }
            } else {
                async_socket->caller_frame = NULL;
            }
            if (LinkAppend(&evlp->send_sockets, async_socket) == -1) {
                RepeatedError("AsyncSend");
                goto DELETE_DEPENDENCY_SEND;
            }
        } else {
            *((int*)async_socket->result_temp) = 0;
        }
        goto NORMAL_END;
    DELETE_DEPENDENCY_SEND:
        LinkDeleteData(&evlp->active_frame->dependency, async_socket);
        goto ERROR_END;
    SEND_DISCONNECT:
        ReleaseError();
        if (DisconnectAsyncSocket(evlp, async_socket) < 0) {
            RepeatedError("CheckBuiltinAndCall");
            goto ERROR_END;
        }
        *((int*)async_socket->result_temp) = -1;
        LinkDeleteData(&evlp->active_frame->dependency, async_socket);
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