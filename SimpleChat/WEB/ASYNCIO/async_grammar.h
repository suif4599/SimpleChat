#ifndef __ASYNC_GRAMMAR_H__
#define __ASYNC_GRAMMAR_H__

#include "../../../ERROR/error.h"
#include "va_func_def.h"

#define ASYNC_LABEL int
#ifdef __linux__
typedef int socket_t;
#define SOCKET_IS_INVALID(s) ((s) < 0)
#elif _WIN32
typedef SOCKET socket_t;
#define SOCKET_IS_INVALID(s) ((s) == INVALID_SOCKET)
#endif

typedef enum {
    NEW_CONNECTION,
    MESSAGE_RECEIVED,
    CONNECTION_CLOSED
} EventType;

typedef struct {
    EventType type;
    void* data;
} Event;

typedef struct {
    char* msg;
    int completed;
} Message;

typedef struct {
    socket_t socket;
    char* ip;
    uint16_t port;
    int is_listen_socket;
    int is_receive_socket;
    int is_ipv6;
    LinkNode* received_messages; // typeof(data) = Message*
} AsyncSocket;

typedef ASYNC_LABEL (*AsyncCallable) (va_list, int, ...);

typedef struct {
    char* name;
    AsyncCallable func;
} AsyncFunction; // User-defined async function

typedef struct {
    AsyncFunction* async_function; // reference to the async function
    ASYNC_LABEL label; // label of the async function
    void* va_data; // __VA_DATA__
    LinkNode* dependency; // list of async functions that this async function depends on
} AsyncFunctionFrame; // Frame of the async function

typedef struct {
    LinkNode* async_functions; // list of async functions, unable to remove, typeof(data) = AsyncFunction*
    LinkNode* async_function_frames; // list of async function frames, typeof(data) = AsyncFunctionFrame*
    void* ret_val; // return value of the async function
    AsyncFunctionFrame* active_frame; // active frame of the async function
    LinkNode* sleep_events; // list of sleep events, typeof(data) = AsyncSleepEvent*
} EventLoop;


#define __ASYNC_GEN_CASE__(n) return n - __ASYNC_RESERVED_ARGUMENT_INNER__; case n:
#define ASYNC_FUNCTION(...) (int __ASYNC_RESERVED_ARGUMENT__, EventLoop* __ASYNC_EVENT_LOOP__, __VA_ARGS__)
#define START_ASYNC_FUNCTION \
    static const int __ASYNC_RESERVED_ARGUMENT_INNER__ = __COUNTER__ + 1; \
    switch(__ASYNC_RESERVED_ARGUMENT__ + __ASYNC_RESERVED_ARGUMENT_INNER__) { case __COUNTER__: \
    __ASYNC_EVENT_LOOP__->ret_val = __VA_DATA__; \
    __ASYNC_GEN_CASE__(__COUNTER__)
#define COROUTINE(f, ...) \
    if (ASYNC_CALL_AND_DEPEND(__ASYNC_EVENT_LOOP__, f, ##__VA_ARGS__) == -1) { \
        AsyncError("AWAIT", "Failed to await the function " #f); \
        return -1; \
    }
#define AWAIT(...) \
    FOREACH(IDENTITY, __VA_ARGS__); \
    __ASYNC_GEN_CASE__(__COUNTER__)
#define END_ASYNC_FUNCTION return 0;}


#define ASYNC_DEF(func, ...) ASYNC_LABEL func VA_DEF_FUNC_SAFE(-1, int, __ASYNC_RESERVED_ARGUMENT__, \
                                                          EventLoop*, __ASYNC_EVENT_LOOP__, __VA_ARGS__) \
                             START_ASYNC_FUNCTION
#define ASYNC_DEF_ONLY(func, ...) ASYNC_LABEL func VA_DEF_ONLY(__VA_RESERVED_ARGUMENT__, __VA_ARGS__)
#define ASYNC_END_DEF END_ASYNC_FUNCTION; VA_END_FUNC(0);
#define ASYNC_ERROR_RETURN(ret_val) va_end(__VA_ARGS_INNER__); return ret_val;

typedef struct {
    int __ASYNC_RESERVED_ARGUMENT__; 
    EventLoop* __ASYNC_EVENT_LOOP__;
} ASYNC_HEADER_STRUCT;


// ASYNC_DEF(f, int, x, char*, s)
//     printf("    f(%d, %s)\n", x, s);
// ASYNC_END_DEF

// ASYNC_DEF(g, int, x, char*, s)
//     printf("    g(%d, %s), Part 1\n", x, s);
//     AWAIT(COROUTINE(f, 2 * x, s));
//     printf("    g(%d, %s), Part 2\n", x, s);
// ASYNC_END_DEF

#endif