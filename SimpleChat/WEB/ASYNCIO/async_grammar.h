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

#include "../../../config.h"
#ifdef __linux__
#include <sys/epoll.h>
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

// typedef struct {
//     char* msg;
//     int completed;
// } Message;

typedef ASYNC_LABEL (*AsyncCallable) (va_list, int, ...);

typedef struct {
    char* name;
    AsyncCallable func;
} AsyncFunction; // User-defined async function

typedef struct {
    AsyncFunction* async_function; // reference to the async function
    ASYNC_LABEL label; // label of the async function
    void* va_data; // __VA_DATA__
    void* saved_var; // __ASYNC_SAVED_VAR__
    LinkNode* dependency; // list of async functions that this async function depends on
} AsyncFunctionFrame; // Frame of the async function

typedef struct {
    socket_t socket;
    char* ip;
    uint16_t port;
    int is_listen_socket;
    int is_receive_socket;
    int is_ipv6;
    int is_closed;
    int is_bound;
    LinkNode* received_messages; // typeof(data) = char*
    char* buffer;
    AsyncFunctionFrame* caller_frame;
    void** result_temp;
    // The following are platform specific
    #ifdef _WIN32
    WSAEVENT event;
    #endif
} AsyncSocket;

typedef struct {
    LinkNode* async_functions; // list of async functions, unable to remove, typeof(data) = AsyncFunction*
    LinkNode* async_function_frames; // list of async function frames, typeof(data) = AsyncFunctionFrame*
    void* ret_val; // return value of the async function
    AsyncFunctionFrame* active_frame; // active frame of the async function
    LinkNode* sleep_events; // list of sleep events, typeof(data) = AsyncSleepEvent*
    LinkNode* bound_sockets; // list of bound async sockets, typeof(data) = AsyncSocket*
    LinkNode* recv_sockets; // list of recv async sockets, typeof(data) = AsyncSocket*
    LinkNode* send_sockets; // list of send async sockets, typeof(data) = AsyncSocket*
    // the following are platform specific
    #ifdef _WIN32
    WSAEVENT events[WSA_MAXIMUM_WAIT_EVENTS];
    AsyncSocket* ev_sockets[WSA_MAXIMUM_WAIT_EVENTS];
    int nEvents;
    #elif __linux__
    int epoll_fd;
    struct epoll_event events[EPOLL_EVENT_NUM];
    #endif
} EventLoop;


#define __ASYNC_GEN_CASE__(n) return n - __ASYNC_RESERVED_ARGUMENT_INNER__; case n:
#define ASYNC_FUNCTION(...) (int __ASYNC_RESERVED_ARGUMENT__, EventLoop* __ASYNC_EVENT_LOOP__, __VA_ARGS__)
#define START_ASYNC_FUNCTION \
    static const int __ASYNC_RESERVED_ARGUMENT_INNER__ = __COUNTER__ + 1; \
    switch(__ASYNC_RESERVED_ARGUMENT__ + __ASYNC_RESERVED_ARGUMENT_INNER__) { case __COUNTER__: \
    __ASYNC_EVENT_LOOP__->ret_val = __VA_DATA__; \
    __ASYNC_GEN_CASE__(__COUNTER__)
/**
 * @brief Generate a coroutine object prepared to be waited by `AWAIT`
 * 
 * @param f the async function
 * @param ... the arguments of the async function
 * 
 * @code
 * ```c
 * // In some async function
 *     AWAIT(
 *         ASYNC_ARG(char*, saved_argument_name),
 *         COROUTINE(async_func_name1, param1, param2),
 *         COROUTINE(async_func_name2, param1, param2, param3)
 *     )
 * ```
 * @endcode
 */
#define COROUTINE(f, ...) (f, ##__VA_ARGS__)
#define _COROUTINE(f, ...) \
    if (ASYNC_CALL_AND_DEPEND(__ASYNC_EVENT_LOOP__, f, ##__VA_ARGS__) == -1) { \
        AsyncError("AWAIT", "Failed to await the coroutine " #f); \
        return -1; \
    }
#define __COROUTINE(args) _COROUTINE args;
#define __SAVE_ASYNC_ARG(struct_ptr, type, name) ((struct_ptr)__ASYNC_SAVED_VAR__)->name = name;
#define __LOAD_ASYNC_ARG(struct_ptr, type, name) name = ((struct_ptr)__ASYNC_SAVED_VAR__)->name;
#define ASYNC_SAVE(...) \
    __ASYNC_SAVED_VAR__ = malloc(sizeof(struct {FOREACH_DOUBLE(STRUCT_SAFE, __VA_ARGS__)})); \
    if (__ASYNC_SAVED_VAR__ == NULL) { \
        MemoryError("ASYNC_SAVE", "Failed to allocate memory for __ASYNC_SAVED_VAR__"); \
        return -1; \
    } \
    FOREACH_DOUBLE_WITH_PREFIX(__SAVE_ASYNC_ARG, struct {FOREACH_DOUBLE(STRUCT_SAFE, __VA_ARGS__)}*, __VA_ARGS__); \
    __ASYNC_EVENT_LOOP__->active_frame->saved_var = __ASYNC_SAVED_VAR__;
#define ASYNC_LOAD(...) \
    __ASYNC_SAVED_VAR__ = __ASYNC_EVENT_LOOP__->active_frame->saved_var; \
    FOREACH_DOUBLE_WITH_PREFIX(__LOAD_ASYNC_ARG, struct {FOREACH_DOUBLE(STRUCT_SAFE, __VA_ARGS__)}*, __VA_ARGS__); \
    free(__ASYNC_SAVED_VAR__); \
    __ASYNC_EVENT_LOOP__->active_frame->saved_var = NULL;
#define __ASYNC_ARG(...) (__VA_ARGS__)
/**
 * @brief Notify the variables that should be keeped during an `AWAIT` block
 * 
 * @param type1 type of the first variable
 * @param name1 name of the first variable
 * @param ... the rest of the variables
 * 
 * @code
 * ```c
 * // In some async function
 *     AWAIT(
 *         ASYNC_ARG(char*, saved_argument_name),
 *         COROUTINE(async_func_name1, param1, param2),
 *         COROUTINE(async_func_name2, param1, param2, param3)
 *     )
 * ```
 * @endcode
 */
#define ASYNC_ARG(...) IF(HAS_ARG(__VA_ARGS__))(__ASYNC_ARG(__VA_ARGS__))(__ASYNC_ARG(int, __ASYNC_RESERVED_ARGUMENT__))
/**
 * @brief Await the coroutines and keep the variables
 * 
 * @param arg the variables that should be keeped
 * @param coro1 the first coroutine
 * @param ... the rest of the coroutines
 * 
 * @code
 * ```c
 * // In some async function
 *     AWAIT(
 *         ASYNC_ARG(char*, saved_argument_name),
 *         COROUTINE(async_func_name1, param1, param2),
 *         COROUTINE(async_func_name2, param1, param2, param3)
 *     )
 * ```
 * @endcode
 * 
 */
#define AWAIT(arg, ...) \
    ASYNC_SAVE arg; \
    FOREACH(__COROUTINE, __VA_ARGS__); \
    __ASYNC_GEN_CASE__(__COUNTER__); \
    ASYNC_LOAD arg;
#define END_ASYNC_FUNCTION return 0;}
#define _ASYNC_CALL_WITH_EVLP(f, ...) \
    if (ASYNC_CALL(__ASYNC_EVENT_LOOP__, f, ##__VA_ARGS__) == -1) { \
        AsyncError("DETACH", "Failed to detach the coroutine " #f); \
        return -1; \
    }
#define __ASYNC_CALL_WITH_EVLP(args) _ASYNC_CALL_WITH_EVLP args;
/**
 * @brief Detach the coroutines
 * 
 * @param coro1 the first coroutine
 * @param ... the rest of the coroutines
 * 
 * @code
 * ```c
 * // In some async function
 *     DETACH(
 *         COROUTINE(async_func_name1, param1, param2),
 *         COROUTINE(async_func_name2, param1, param2, param3)
 *     )
 * ```
 * @endcode
 * 
 */
#define DETACH(...) \
    FOREACH(__ASYNC_CALL_WITH_EVLP, __VA_ARGS__);

/**
 * @brief Define an async function
 * 
 * @param func name of the async function
 * @param type1 type of the first parameter
 * @param name1 name of the first parameter
 * @param ... the rest of the parameters
 * 
 * @code
 * ```c
 * ASYNC_DEF(say, int, delay, char*, s)
 *     AWAIT(
 *         ASYNC_ARG(),
 *         COROUTINE(AsyncSleep, delay)
 *     );
 *     printf("    Say after %dms: %s\n", delay, s);
 * ASYNC_END_DEF
 * 
 * ASYNC_DEF(g, char*, s)
 *     printf("    Begin: %s\n", s);
 *     char* str = malloc(128);
 *     sprintf(str, "Hello, %s", s);
 *     AWAIT(
 *         ASYNC_ARG(char*, str),
 *         COROUTINE(say, 1000, s),
 *         COROUTINE(say, 2000, str)
 *     )
 *     free(str);
 *     printf("    End: %s\n", s);
 * ASYNC_END_DEF
 * ```
 * @endcode
 * 
 */
#define ASYNC_DEF(func, ...) ASYNC_LABEL func VA_DEF_FUNC_SAFE(-1, int, __ASYNC_RESERVED_ARGUMENT__, \
                                                          EventLoop*, __ASYNC_EVENT_LOOP__, __VA_ARGS__) \
                             void* __ASYNC_SAVED_VAR__; \
                             START_ASYNC_FUNCTION
#define ASYNC_DEF_ONLY(func, ...) ASYNC_LABEL func VA_DEF_ONLY(__VA_RESERVED_ARGUMENT__, __VA_ARGS__)
#define ASYNC_END_DEF END_ASYNC_FUNCTION; VA_END_FUNC(0);
// #define ASYNC_ERROR_RETURN(ret_val) va_end(__VA_ARGS_INNER__); return ret_val;
#define ASYNC_RETURN(ret_val) VA_RETURN_SAFE(ret_val);

#define ASYNC_DATA_STRUCT(...) VA_DATA_STRUCT(int, __ASYNC_RESERVED_ARGUMENT__, \
                                            EventLoop*, __ASYNC_EVENT_LOOP__, __VA_ARGS__)

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