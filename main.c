#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SimpleChat/WEB/ASYNCIO/asyncio.h"
#include "ERROR/error.h"
#include "MACRO/macro.h"

// sudo ps -a | grep SimpleChat | while IFS= read -r line; do pid=$(echo $line | awk '{print $1}'); echo "Killing ${pid}"; sudo kill -9 $pid; done


ASYNC_DEF(serverAccept, AsyncSocket*, server_socket)
    while (1) {
        printf("    Waiting for connection...\n");
        static AsyncSocket* client_socket;
        AWAIT(
            ASYNC_ARG(),
            COROUTINE(AsyncAccept, server_socket, &client_socket)
        );
        if (client_socket == NULL) {
            printf("    Connection failed\n");
            continue;
        }
        printf("    Connection established: (%s:%d)\n", client_socket->ip, client_socket->port);
        DETACH(
            COROUTINE(serverRecv, client_socket)
        );
    }
ASYNC_END_DEF

ASYNC_DEF(serverRecv, AsyncSocket*, client_socket)
    while (1) {
        static char* msg;
        printf("    Waiting for message of (%s:%d)...\n", client_socket->ip, client_socket->port);
        AWAIT(
            ASYNC_ARG(),
            COROUTINE(AsyncRecv, client_socket, &msg)
        );
        if (msg == NULL) {
            printf("    Connection closed\n");
            break;
        }
        printf("    Received from (%s:%d): %s\n", client_socket->ip, client_socket->port, msg);
        free(msg);
    }
ASYNC_END_DEF

int __main(uint16_t port) {
    EventLoop* evlp = CreateEventLoop();
    if (evlp == NULL) goto ERROR_END;
    if (RegisterAsyncFunction(evlp, serverAccept) == -1) goto ERROR_END;
    if (RegisterAsyncFunction(evlp, serverRecv) == -1) goto ERROR_END;
    AsyncSocket* server_socket = CreateAsyncSocket("0.0.0.0", port, 5000, 5000, 1, 0, 0);
    if (server_socket == NULL) goto ERROR_END;
    if (ASYNC_CALL(evlp, serverAccept, server_socket) == -1) goto ERROR_END;
    // if (ASYNC_CALL(evlp, serverRecv) == -1) goto ERROR_END;
    if (EventLoopRun(evlp, 100) == -1) goto ERROR_END;
    return 0;
ERROR_END:
    RepeatedError("main");
    if (ReleaseEventLoop(evlp) == -1) {
        RepeatedError("main.release");
    }
    return -1;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: <port>\n");
        return -1;
    }
    uint16_t port = atoi(argv[1]);
    if (InitErrorStream() < 0) {
        PrintError();
        return -1;
    }
    if (__main(port) == -1) {
        PrintError();
        return -1;
    }
    return 0;
}