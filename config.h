#ifndef __CONFIG_H__
#define __CONFIG_H__

#define ASYNC_RECV_BUFFER_SIZE 300
#define STDIN_BUFFER_SIZE 300
#define MAX_NAME_LENGTH 50

#ifdef __linux__
#define EPOLL_EVENT_NUM 128
#endif

// #define SHOW_FUNCTION_CALL
#define ACTIVATE_ERROR_STREAM
// #define SHORT_EVAL

#endif