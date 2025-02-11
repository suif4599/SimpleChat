#include "asyncio.h"

AsyncFunction* CreateAsyncFunction(const char* name, AsyncCallable func) {
    AsyncFunction* async_function = (AsyncFunction*)malloc(sizeof(AsyncFunction));
    if (async_function == NULL) {
        MemoryError("CreateAsyncFunction", "Failed to allocate memory for async_function");
        return NULL;
    }
    async_function->name = (char*)malloc(strlen(name) + 1);
    if (async_function->name == NULL) {
        MemoryError("CreateAsyncFunction", "Failed to allocate memory for name");
        free(async_function);
        return NULL;
    }
    strcpy(async_function->name, name);
    async_function->func = func;
    return async_function;
}

void ReleaseAsyncFunction(AsyncFunction* async_function) {
    if (async_function->name != NULL) free(async_function->name);
    free(async_function);
}

AsyncFunctionFrame* CreateAsyncFunctionFrame(AsyncFunction* async_function) {
    AsyncFunctionFrame* async_function_frame = (AsyncFunctionFrame*)malloc(sizeof(AsyncFunctionFrame));
    if (async_function_frame == NULL) {
        MemoryError("CreateAsyncFunctionFrame", "Failed to allocate memory for async_function_frame");
        return NULL;
    }
    async_function_frame->async_function = async_function;
    async_function_frame->label = 0;
    async_function_frame->va_data = NULL;
    async_function_frame->dependency = NULL;
    return async_function_frame;
}

void ReleaseAsyncFunctionFrame(AsyncFunctionFrame* async_function_frame) {
    if (async_function_frame->va_data != NULL) free(async_function_frame->va_data);
    free(async_function_frame);
}

#define SET_VA_DATA_LABEL(vadata, label) \
    ((ASYNC_HEADER_STRUCT*)(vadata))->__ASYNC_RESERVED_ARGUMENT__ = label;

ASYNC_LABEL CallAsyncFunctionFrame(EventLoop* event_loop, AsyncFunctionFrame* async_function_frame) {
    if (async_function_frame->dependency != NULL) return -2;
    event_loop->active_frame = async_function_frame;
    SET_VA_DATA_LABEL(async_function_frame->va_data, async_function_frame->label);
    ASYNC_LABEL ret = VA_CALL_WITH_VADATA(async_function_frame->async_function->func, async_function_frame->va_data);
    if (ret == -1) {
        AsyncError("CallAsyncFunctionFrame", "Failed to call the async function");
        event_loop->active_frame = NULL;
        return -1;
    }
    async_function_frame->label = ret;
    if (ret == 0) {
        RemoveFrameDependencyFromEventLoop(event_loop, async_function_frame);
        ReleaseAsyncFunctionFrame(async_function_frame);
    }
    event_loop->active_frame = NULL;
    return ret;
}