#include "asyncio.h"
#include <stdio.h>

int AsyncIOInit() {
    #ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        SystemError("AsyncIOInit", "Failed to start up WSA");
        return -1;
    }
    #endif
    return 0;
}

EventLoop* CreateEventLoop() {
    EventLoop* event_loop = malloc(sizeof(EventLoop));
    if (event_loop == NULL) {
        MemoryError("CreateEventLoop", "Failed to allocate memory for EventLoop");
        return NULL;
    }
    event_loop->async_functions = NULL;
    event_loop->async_function_frames = NULL;
    event_loop->ret_val = NULL;
    event_loop->active_frame = NULL;
    event_loop->sleep_events = NULL;
    event_loop->bound_sockets = NULL;
    event_loop->recv_sockets = NULL;
    // the following are platform specific
    #ifdef _WIN32
    event_loop->nEvents = 0;
    #elif __linux__
    event_loop->epoll_fd = epoll_create(1);
    if (event_loop->epoll_fd == -1) {
        EpollError("CreateEventLoop", "Failed to create epoll fd");
        goto RELEASE_EVENT_LOOP;
    }
    #endif
    return event_loop;
RELEASE_EVENT_LOOP:
    free(event_loop);
    return NULL;
}

int __RegisterAsyncFunction(EventLoop* event_loop, AsyncCallable async_function, const char* name) {
    AsyncFunction* func = CreateAsyncFunction(name, async_function);
    if (func == NULL) {
        RepeatedError("RegisterAsyncFunction");
        return -1;
    }
    if (LinkAppend(&event_loop->async_functions, func) == -1) {
        RepeatedError("RegisterAsyncFunction");
        goto RELEASE_ASYNC_FUNCTION;
    }
    return 0;
RELEASE_ASYNC_FUNCTION:
    ReleaseAsyncFunction(func);
    return -1;
}



int __CallAsyncFunction(EventLoop* evlp, const char* func_name, int add_depend, ...) {
    va_list args; // 0, evlp and other arguments
    va_start(args, add_depend);
    // printf("[__CallAsyncFunction]: %s\n", func_name);
    int ret = CheckBuiltinAndCall(func_name, add_depend, args);
    if (ret == -1) {
        RepeatedError("__CallAsyncFunction");
        va_end(args);
        return -1;
    }
    if (ret == 0) {
        va_end(args);
        return 0;
    }
    AsyncFunction* func = GetAsyncFunctionByName(evlp, func_name);
    if (func == NULL) {
        AsyncError("__CallAsyncFunction", "Failed to get the async function");
        return -1;
    }
    AsyncFunctionFrame* frame = CreateAsyncFunctionFrame(func);
    if (frame == NULL) {
        RepeatedError("__CallAsyncFunction");
        goto RELEASE_VA_DATA;
    }
    ASYNC_LABEL n = VA_CALL_WITH_VALIST(func->func, args);
    if (n == -1) {
        AsyncError("__CallAsyncFunction", "Failed to call the async function");
        return -1;
    }
    void* __VA_DATA__ = evlp->ret_val;
    evlp->ret_val = NULL;
    frame->va_data = __VA_DATA__;
    frame->label = n;
    if (LinkAppend(&evlp->async_function_frames, frame) == -1) {
        RepeatedError("__CallAsyncFunction");
        goto RELEASE_FRAME;
    }
    if (add_depend) {
        if (LinkAppend(&evlp->active_frame->dependency, frame) == -1) {
            RepeatedError("__CallAsyncFunction");
            goto RELEASE_FRAME;
        }
    }
    va_end(args);
    return 0;
RELEASE_FRAME:
    ReleaseAsyncFunctionFrame(frame);
RELEASE_VA_DATA:
    free(__VA_DATA__);
    va_end(args);
    return -1;
}

AsyncFunction* GetAsyncFunctionByName(EventLoop* event_loop, const char* name) {
    LinkNode* node = event_loop->async_functions;
    while (node != NULL) {
        AsyncFunction* func = (AsyncFunction*)node->data;
        if (strcmp(func->name, name) == 0) {
            return func;
        }
        node = node->next;
    }
    NameError("GetAsyncFunctionByName", "Failed to find the async function");
    return NULL;
}
/**
 * @brief it won't remove frame from event_loop->async_function_frames
*/
void RemoveFrameDependencyFromEventLoop(EventLoop* event_loop, AsyncFunctionFrame* frame) {
    LinkNode* frame_node = event_loop->async_function_frames;
    while (frame_node != NULL) {
        AsyncFunctionFrame* f = (AsyncFunctionFrame*)frame_node->data;
        if (f == frame) {
            frame_node = frame_node->next;
            continue;
        }
        LinkNode* dependency_node = f->dependency;
        while (dependency_node != NULL) {
            if (dependency_node->data == (void*)frame) {
                dependency_node = LinkDeleteNode(&f->dependency, dependency_node);
                continue;
            }
            dependency_node = dependency_node->next;
        }
        frame_node = frame_node->next;
    }
}

static int parseMessage(AsyncSocket* sock) {
    const int HEADER_LEN = strlen(ASYNC_MSG_HEADER) + 2;
    const int FOOTER_LEN = strlen(ASYNC_MSG_HEADER) + 3;
    if (sock->buffer == NULL) {
        return 0;
    }
    int i = 0;
    int len = strlen(sock->buffer);
    // printf("[parseMessage]: raw = %s\n", sock->buffer);
    while (i < len - HEADER_LEN) {
        if (strncmp(sock->buffer + i, "<" ASYNC_MSG_HEADER ">", HEADER_LEN) != 0) {
            i++;
            continue;
        }
        int j = i + HEADER_LEN;
        while (j <= len - FOOTER_LEN) {
            if (strncmp(sock->buffer + j, "</" ASYNC_MSG_HEADER ">", FOOTER_LEN) != 0) {
                j++;
                continue;
            }
            char* message = malloc(j - i - HEADER_LEN + 1);
            if (message == NULL) {
                MemoryError("parseMessage", "Failed to allocate memory for message");
                return -1;
            }
            strncpy(message, sock->buffer + i + HEADER_LEN, j - i - HEADER_LEN);
            message[j - i - HEADER_LEN] = '\0';
            if (LinkAppend(&sock->received_messages, message) < 0) {
                RepeatedError("parseMessage");
                free(message);
                return -1;
            }
            char* temp = malloc(len - j - FOOTER_LEN + 1);
            if (temp == NULL) {
                MemoryError("parseMessage", "Failed to allocate memory for temp");
                free(message);
                return -1;
            }
            strcpy(temp, sock->buffer + j + FOOTER_LEN);
            free(sock->buffer);
            sock->buffer = temp;
            return 0;
        }
        return 0;
    }
    return 0;
}

#define __ACCEPT_RESULT(s) *((AsyncSocket**)(s->result_temp))
static int handleSocketEvent(EventLoop* event_loop, AsyncSocket* sock) {
    // printf("[handleSocketEvent]: (%s:%d)\n", sock->ip, sock->port);
    if (sock->is_listen_socket) { // corresponding to AsyncAccept
        UnbindAsyncSocket(event_loop, sock);
        struct sockaddr addr;
        socklen_t addr_len = sizeof(addr);
        int client_socket_fd = accept(sock->socket, &addr, &addr_len);
        if SOCKET_IS_INVALID(client_socket_fd) {
            __ACCEPT_RESULT(sock) = NULL;
            return 0;
        }
        AsyncSocket* client_socket = CreateAsyncRecvSocketFromSocket(client_socket_fd, addr);
        if (client_socket == NULL) {
            __ACCEPT_RESULT(sock) = NULL;
            return 0;
        }
        if (UnbindAsyncSocket(event_loop, sock) < 0) {
            ReleaseAsyncSocket(client_socket);
            __ACCEPT_RESULT(sock) = NULL;
            RepeatedError("handleSocketEvent");
            return -1; // rarely happens
        }
        if (BindAsyncSocket(event_loop, client_socket) < 0) {
            ReleaseAsyncSocket(client_socket);
            __ACCEPT_RESULT(sock) = NULL;
            RepeatedError("handleSocketEvent");
            return -1; // rarely happens
        } 
        __ACCEPT_RESULT(sock) = client_socket;
        if (sock->caller_frame != NULL) {
            LinkDeleteData(&sock->caller_frame->dependency, sock);
            sock->caller_frame = NULL;
        }
        return 0;
    }
    if (sock->is_receive_socket) { // corresponding to sock->buffer and sock->sock->received_messages
        char* buffer;
        if (sock->buffer == NULL) {
            sock->buffer = malloc(ASYNC_RECV_BUFFER_SIZE);
            if (sock->buffer == NULL) {
                MemoryError("handleSocketEvent", "Failed to allocate memory for buffer");
                return -1;
            }
            buffer = sock->buffer;
        } else {
            int len = (int)strlen(sock->buffer); // message cannot be too long
            sock->buffer = realloc(sock->buffer, len + ASYNC_RECV_BUFFER_SIZE);
            if (sock->buffer == NULL) {
                MemoryError("handleSocketEvent", "Failed to reallocate memory for buffer");
                return -1;
            }
            buffer = sock->buffer + len;
        }
        int ret_len = recv(sock->socket, buffer, ASYNC_RECV_BUFFER_SIZE - 1, 0);
        if (ret_len <= 0) {
            // Fully remove the socket from the event loop
            if (DisconnectAsyncSocket(event_loop, sock) < 0) {
                RepeatedError("handleSocketEvent");
                return -1; // rarely happens
            }
            return 0;
        }
        buffer[ret_len] = '\0';
        return 0;
    }
}

void show_all_frame(EventLoop* event_loop) {
    printf("[show_all_frame]: begin\n");
    LinkNode* node = event_loop->async_function_frames;
    while (node != NULL) {
        AsyncFunctionFrame* frame = (AsyncFunctionFrame*)node->data;
        printf("[show_all_frame]: frame for <%s>\n", frame->async_function->name);
        node = node->next;
    }
    printf("[show_all_frame]: end\n");
}
#define __RECV_RESULT(s) *((char**)(s->result_temp))
int EventLoopRun(EventLoop* event_loop, int delay) {
    int flag; // if in one loop something happens, then the delay will be skipped
    LinkNode* node;
    while (1) {
        // printf("[EventLoopRun]: begin\n");
        node = event_loop->async_function_frames;
        flag = 0;
        if (RefreshErrorStream() < 0) {
            RepeatedError("ServerMainloop");
            return -1;
        }
        // show_all_frame(event_loop);
        { // Main Event Loop
            // printf("[EventLoopRun]: Main Event Loop\n");
            while (node != NULL) {
                AsyncFunctionFrame* frame = (AsyncFunctionFrame*)node->data;
                LinkNode* next = node->next;
                ASYNC_LABEL ret = CallAsyncFunctionFrame(event_loop, frame); // return -2 if it is blocked, return 0 if the frame is released
                // printf("[EventLoopRun]: ret of = %d\n", ret);
                if (ret == -1) {
                    char buffer[128];
                    sprintf(buffer, "EventLoopRun (when calling <%s>)", frame->async_function->name);
                    RepeatedError(buffer);
                    return -1;
                }
                if (ret == -2) { // blocked
                    node = next;
                    continue;
                }
                flag = 1;
                if (ret == 0) { // frame is released
                    node = LinkDeleteNode(&event_loop->async_function_frames, node);
                    // printf("[EventLoopRun]: frame is released\n");
                    continue;
                }
                node = next;
            }
        }

        { // Sleep Events
            // printf("[EventLoopRun]: Sleep Events\n");
            if (event_loop->sleep_events != NULL) {
                long long current_time = currentTimeMillisec();
                LinkNode* sleep_node = event_loop->sleep_events;
                while (sleep_node != NULL) {
                    AsyncSleepEvent* event = (AsyncSleepEvent*)sleep_node->data;
                    if (event->target_time <= current_time) { // Done
                        flag = 1;
                        if (event->caller_frame != NULL) {
                            LinkNode* depend_node = event->caller_frame->dependency;
                            while (depend_node != NULL) {
                                if (depend_node->data == (void*)event) {
                                    LinkDeleteNode(&event->caller_frame->dependency, depend_node);
                                    break;
                                }
                                depend_node = depend_node->next;
                            }
                        }
                        sleep_node = LinkDeleteNode(&event_loop->sleep_events, sleep_node);
                        continue;
                    }
                    sleep_node = sleep_node->next;
                }
            }
        }

        { // Epoll Events (linux)
            // printf("[EventLoopRun]: Epoll Events\n");
            #ifdef __linux__
            int nfds = epoll_wait(event_loop->epoll_fd, event_loop->events, EPOLL_EVENT_NUM, 0);
            if (nfds < 0) {
                EpollError("EventLoopRun", "Failed to wait for events");
                return -1;
            }
            if (nfds > 0) {
                flag = 1;
                for (int i = 0; i < nfds; i++) {
                    LinkNode* socket_node = event_loop->bound_sockets;
                    while (socket_node != NULL) {
                        AsyncSocket* socket = socket_node->data;
                        if (socket == event_loop->events[i].data.ptr) {
                            if (handleSocketEvent(event_loop, socket) < 0) {
                                RepeatedError("EventLoopRun");
                                return -1;
                            }
                            break;
                        }
                        socket_node = socket_node->next;
                    }
                }
            }
            #endif
        }

        { // WSAEventSelect (windows)
            // printf("[EventLoopRun]: WSAEventSelect\n");
            #ifdef _WIN32
            if (event_loop->nEvents > 0) {
                DWORD index = WSAWaitForMultipleEvents(event_loop->nEvents, event_loop->events, FALSE, 0, FALSE);
                if (index == WSA_WAIT_FAILED) {
                    EventError("EventLoopRun", "Failed to wait for events");
                    return -1;
                }
                if (index != WSA_WAIT_TIMEOUT) {
                    flag = 1;
                    index -= WSA_WAIT_EVENT_0;
                    int flag = 1;
                    if (WSAResetEvent(event_loop->events[index]) == FALSE) {
                        EventError("EventLoopRun", "Failed to reset the event");
                        return -1;
                    }
                    if (flag) {
                        if (handleSocketEvent(event_loop, event_loop->ev_sockets[index]) < 0) {
                            RepeatedError("EventLoopRun");
                            return -1;
                        }
                    }
                }
            }
            #endif
        }

        { // Recv Events
        // NOTE: maybe recv_node will contain a invalid pointer
            // printf("[EventLoopRun]: Recv Events\n");
            LinkNode* recv_node = event_loop->recv_sockets;
            while (recv_node != NULL) {
                // printf("[EventLoopRun]: Recv Events, sockfd = %d\n", ((AsyncSocket*)recv_node->data)->socket);
                AsyncSocket* sock = recv_node->data;
                if (sock->is_closed) {
                    // printf("[EventLoopRun]: socket is closed, ip = %s, port = %d\n", sock->ip, sock->port);
                    __RECV_RESULT(sock) = NULL;
                    if (sock->caller_frame != NULL) {
                        LinkDeleteData(&sock->caller_frame->dependency, sock);
                        sock->caller_frame = NULL;
                    }
                    ReleaseAsyncSocket(sock);
                    recv_node = LinkDeleteNode(&event_loop->recv_sockets, recv_node);
                    continue;
                }
                if (parseMessage(sock) < 0) {
                    RepeatedError("EventLoopRun");
                    return -1;
                }
                if (sock->received_messages != NULL) {
                    flag = 1;
                    LinkNode* message_node = sock->received_messages;
                    sock->received_messages = sock->received_messages->next;
                    char* message = message_node->data;
                    free(message_node);
                    __RECV_RESULT(sock) = message;
                    if (sock->caller_frame != NULL) {
                        LinkDeleteData(&sock->caller_frame->dependency, sock);
                        sock->caller_frame = NULL;
                    }
                    LinkDeleteData(&event_loop->recv_sockets, sock);
                }
                recv_node = recv_node->next;
            }
        }
        // printf("[EventLoopRun]: end\n");
        if (flag) continue;
        #ifdef _WIN32
        Sleep(delay);
        #elif __linux__
        usleep(delay * 1000);
        #endif
    }
}

int ReleaseEventLoop(EventLoop* event_loop) {
    #ifdef _WIN32
    #pragma message("Warning: Not implemented")
    #elif __linux__
    #warning "Not implemented"
    #endif
    return 0;
}

