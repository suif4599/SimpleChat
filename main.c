#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SimpleChat/WEB/ASYNCIO/asyncio.h"
#include "ERROR/error.h"
#include "MACRO/macro.h"

// sudo ps -a | grep SimpleChat | while IFS= read -r line; do pid=$(echo $line | awk '{print $1}'); echo "Killing ${pid}"; sudo kill -9 $pid; done


ASYNC_DEF(serverAccept, AsyncSocket*, server_socket)
    printf("    Waiting for connection at (%s:%d)...\n", server_socket->ip, server_socket->port);
    while (1) {
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
    printf("    Waiting for message of (%s:%d)...\n", client_socket->ip, client_socket->port);
    while (1) {
        static char* msg;
        AWAIT(
            ASYNC_ARG(),
            COROUTINE(AsyncRecv, client_socket, &msg)
        );
        if (msg == NULL) {
            printf("    Connection closed\n");
            break;
        }
        if (strlen(msg) > 1024) {
            printf("    Received from (%s:%d): len = %d\n", client_socket->ip, client_socket->port, (int)strlen(msg));
        } else
            printf("    Received from (%s:%d): %s\n", client_socket->ip, client_socket->port, msg);
        free(msg);
    }
ASYNC_END_DEF

int callMain(uint16_t port) {
    if (AsyncIOInit() == -1) {
        RepeatedError("main");
        return -1;
    }
    EventLoop* evlp = CreateEventLoop();
    if (evlp == NULL) goto ERROR_END;
    if (RegisterAsyncFunction(evlp, serverAccept) == -1) goto ERROR_END;
    if (RegisterAsyncFunction(evlp, serverRecv) == -1) goto ERROR_END;
    AsyncSocket* server_socket = CreateAsyncSocket("0.0.0.0", port, 5000, 5000, 1, 0, 0);
    if (server_socket == NULL) goto ERROR_END;
    if (ASYNC_CALL(evlp, serverAccept, server_socket) == -1) goto ERROR_END;
    // if (ASYNC_CALL(evlp, serverRecv) == -1) goto ERROR_END;
    if (EventLoopRun(evlp, 100) == -1) goto ERROR_END;
    if (ReleaseEventLoop(evlp) < 0) goto ERROR_END;
    return 0;
ERROR_END:
    RepeatedError("main");
    if (ReleaseEventLoop(evlp) == -1) {
        RepeatedError("main.release");
    }
    return -1;
}

ASYNC_DEF(clientSend, AsyncSocket*, client_socket, char*, msg)
    printf("    Sending to (%s:%d):\n", client_socket->ip, client_socket->port);
    static int result;
    int round = 0;
    char* temp = malloc(strlen(msg) + 2 * strlen(ASYNC_MSG_HEADER) + 10);
    if (temp == NULL) {
        MemoryError("clientSend", "Failed to allocate memory for temp");
        ASYNC_RETURN(-1);
    }
    sprintf(temp, "<%s>%s</%s>", ASYNC_MSG_HEADER, msg, ASYNC_MSG_HEADER);
    while (1) {
        round++;
        printf("    Round %d\n", round);
        AWAIT(
            ASYNC_ARG(int, round, char*, temp),
            COROUTINE(AsyncSend, client_socket, temp, &result),
            COROUTINE(AsyncSleep, 1000)
        );
        if (result == -1) {
            printf("    Connection closed\n");
            break;
        }
    }
ASYNC_END_DEF


int sendMain(char* ip, uint16_t port) {
    char msg[] = "Hello, World!";
    if (AsyncIOInit() == -1) {
        RepeatedError("main");
        return -1;
    }
    EventLoop* evlp = CreateEventLoop();
    if (evlp == NULL) goto ERROR_END;
    if (RegisterAsyncFunction(evlp, clientSend) == -1) goto ERROR_END;
    AsyncSocket* client_socket = CreateAsyncSocket(ip, port, 5000, 5000, 0, 0, 0);
    if (client_socket == NULL) goto ERROR_END;
    if (ASYNC_CALL(evlp, clientSend, client_socket, msg) == -1) goto ERROR_END;
    if (EventLoopRun(evlp, 100) == -1) goto ERROR_END;
    if (ReleaseEventLoop(evlp) < 0) goto ERROR_END;
    return 0;
ERROR_END:
    RepeatedError("main");
    if (ReleaseEventLoop(evlp) == -1) {
        RepeatedError("main.release");
    }
    return -1;
}

int main(int argc, char* argv[]) {
    if (argc != 2 && argc != 4) {
        printf("Usage: SimpleChat <port>, or SimpleChat -s <ip> <port>\n");
        return -1;
    }
    uint16_t port;
    if (argc == 2)
        port = atoi(argv[1]);
    else
        port = atoi(argv[3]);
    if (InitErrorStream() < 0) {
        PrintError();
        return -1;
    }
    if (argc == 2) {
        if (callMain(port) == -1) {
            PrintError();
            return -1;
        }
    } else {
        if (sendMain(argv[2], port) == -1) {
            PrintError();
            return -1;
        }
    }
    return 0;
}