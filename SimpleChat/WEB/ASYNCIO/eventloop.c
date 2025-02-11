#include "asyncio.h"

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
    return event_loop;
// RELEASE_EVENT_LOOP:
//     free(event_loop);
//     return NULL;
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

void RemoveFrameDependencyFromEventLoop(EventLoop* event_loop, AsyncFunctionFrame* frame) {
    LinkNode* frame_node = event_loop->async_function_frames;
    while (frame_node != NULL) {
        AsyncFunctionFrame* f = (AsyncFunctionFrame*)frame_node->data;
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

// void show_all_frame(EventLoop* event_loop) {
//     printf("[show_all_frame]: begin\n");
//     LinkNode* node = event_loop->async_function_frames;
//     while (node != NULL) {
//         AsyncFunctionFrame* frame = (AsyncFunctionFrame*)node->data;
//         printf("[show_all_frame]: frame for <%s>\n", frame->async_function->name);
//         node = node->next;
//         printf("[show_all_frame]: next, is_null = %d\n", node == NULL);
//     }
//     printf("[show_all_frame]: end\n");
// }

int EventLoopRun(EventLoop* event_loop, int delay) {

    while (1) {
        LinkNode* node = event_loop->async_function_frames;
        // show_all_frame(event_loop);
        while (node != NULL) {
            AsyncFunctionFrame* frame = (AsyncFunctionFrame*)node->data;
            ASYNC_LABEL ret = CallAsyncFunctionFrame(event_loop, frame); // return -2 if it is blocked
            if (ret == -1) {
                RepeatedError("EventLoopRun");
                return -1;
            }
            if (ret == 0) {
                node = LinkDeleteNode(&event_loop->async_function_frames, node);
                continue;
            }
            node = node->next;
        }
        if (event_loop->sleep_events != NULL) {
            long long current_time = currentTimeMillisec();
            LinkNode* sleep_node = event_loop->sleep_events;
            while (sleep_node != NULL) {
                AsyncSleepEvent* event = (AsyncSleepEvent*)sleep_node->data;
                if (event->target_time <= current_time) { // Done
                    LinkNode* depend_node = event->caller_frame->dependency;
                    while (depend_node != NULL) {
                        if (depend_node->data == (void*)event) {
                            LinkDeleteNode(&event->caller_frame->dependency, depend_node);
                            break;
                        }
                        depend_node = depend_node->next;
                    }
                    sleep_node = LinkDeleteNode(&event_loop->sleep_events, sleep_node);
                    continue;
                }
                sleep_node = sleep_node->next;
            }
        }
        #ifdef _WIN32
        Sleep(delay);
        #elif __linux__
        usleep(delay * 1000);
        #endif
    }
}

int ReleaseEventLoop(EventLoop* event_loop) {
    #ifdef _WIN32
    #programa message("Warning: Not implemented")
    #elif __linux__
    #warning "Not implemented"
    #endif
    return 0;
}

