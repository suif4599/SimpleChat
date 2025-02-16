#ifndef __ASYNCIO_H__
#define __ASYNCIO_H__

/* if a function returns <int>, then 0 means success, -1 means error
 * if a function returns <pointer>, then NULL means error
 */

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdint.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif __linux__
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/time.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "../TOOLS/link.h"
#include "../../../MACRO/macro.h"
#include "async_grammar.h"



int AsyncSleep(int millisec);



long long currentTimeMillisec();

typedef struct {
    long long target_time;
    AsyncFunctionFrame* caller_frame;
} AsyncSleepEvent;



int AsyncIOInit();

int CheckBuiltinAndCall(const char* func_name, int add_depend, va_list args); 
    // return -1 if error, 0 if found, 1 if not found


AsyncSocket* CreateAsyncSocket(const char* ip, uint16_t port, int send_time_out, int recv_time_out, int listen_mode, int receive_mode, int use_ipv6);
    // in milliseconds
AsyncSocket* CreateAsyncRecvSocketFromSocket(socket_t socket, struct sockaddr addr);
int ReleaseAsyncSocket(AsyncSocket* async_socket);
/**
 * @brief Accept the client socket **and bind it**, the client socket would be a receive socket
 * 
 * @param async_socket 
 * @param result_socket 
 * @return (*result_socket) may be NULL
 * 
 */
void AsyncAccept(AsyncSocket* async_socket, AsyncSocket** result_socket); // accept the client socket **and bind it**
/**
 * @brief Receive one message from the socket
 * 
 * @param async_socket 
 * @param msg 
 * @return (*msg) 
 */
void AsyncRecv(AsyncSocket* async_socket, char** msg);

AsyncFunction* CreateAsyncFunction(const char* name, AsyncCallable func);
void ReleaseAsyncFunction(AsyncFunction* async_function);


AsyncFunctionFrame* CreateAsyncFunctionFrame(AsyncFunction* async_function);
void ReleaseAsyncFunctionFrame(AsyncFunctionFrame* async_function_frame); // it won't release the async function
ASYNC_LABEL CallAsyncFunctionFrame(EventLoop* event_loop, AsyncFunctionFrame* async_function_frame); 
    // it would release the frame if it returns properly
    // return -2 if it is blocked


EventLoop* CreateEventLoop();
int ReleaseEventLoop(EventLoop* event_loop);
int BindAsyncSocket(EventLoop* event_loop, AsyncSocket* async_socket);
int UnbindAsyncSocket(EventLoop* event_loop, AsyncSocket* async_socket);
int RemoveAsyncSocket(EventLoop* event_loop, AsyncSocket* async_socket);
int __RegisterAsyncFunction(EventLoop* event_loop, AsyncCallable async_function, const char* name);
#define RegisterAsyncFunction(evlp, func) __RegisterAsyncFunction(evlp, func, #func)
int EventLoopRun(EventLoop* event_loop, int delay); // in milliseconds
int RegisterAsyncFunctionFrame(EventLoop* event_loop, AsyncFunctionFrame* async_function_frame);
AsyncFunction* GetAsyncFunctionByName(EventLoop* event_loop, const char* name);
// int __CallAsyncFunction(int, ...);
int __CallAsyncFunction(EventLoop* evlp, const char* func_name, int add_depend, ...);
#define ASYNC_CALL(evlp, func, ...) __CallAsyncFunction(evlp, #func, 0, 0, evlp, ##__VA_ARGS__)
#define ASYNC_CALL_AND_DEPEND(evlp, func, ...) __CallAsyncFunction(evlp, #func, 1, 0, evlp, ##__VA_ARGS__)
void RemoveFrameDependencyFromEventLoop(EventLoop* event_loop, AsyncFunctionFrame* frame);

int DisconnectAsyncSocket(EventLoop* event_loop, AsyncSocket* async_socket);


#define ASYNC_MSG_HEADER "ASYNC_MSG_852746"
#define GEN_ASYNC_MSG(msg) "<" ASYNC_MSG_HEADER ">" msg "</" ASYNC_MSG_HEADER ">"

#endif