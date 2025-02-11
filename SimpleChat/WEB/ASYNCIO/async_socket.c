#include "asyncio.h"

AsyncSocket* CreateAsyncSocket(const char* ip, uint16_t port, int listen_mode, int receive_mode, int use_ipv6) {
    AsyncSocket* wrapper = (AsyncSocket*)malloc(sizeof(AsyncSocket));
    if (wrapper == NULL) {
        MemoryError("CreateAsyncSocket", "Failed to allocate memory for wrapper");
        return NULL;
    }
    wrapper->is_listen_socket = listen_mode;
    wrapper->is_receive_socket = receive_mode;
    wrapper->is_ipv6 = use_ipv6;
    if (use_ipv6) {
        wrapper->socket = socket(AF_INET6, SOCK_STREAM, 0);
    } else {
        wrapper->socket = socket(AF_INET, SOCK_STREAM, 0);
    }
    if (SOCKET_IS_INVALID(wrapper->socket)) {
        SocketError("CreateAsyncSocket", "Failed to create socket");
        goto RELEASE_WRAPPER;
    }
    struct sockaddr addr;
    wrapper->is_ipv6 = use_ipv6;
    if (use_ipv6) {
        ((struct sockaddr_in6*)&addr)->sin6_family = AF_INET6;
        ((struct sockaddr_in6*)&addr)->sin6_port = htons(port);
        inet_pton(AF_INET6, ip, &((struct sockaddr_in6*)&addr)->sin6_addr);
    } else {
        ((struct sockaddr_in*)&addr)->sin_family = AF_INET;
        ((struct sockaddr_in*)&addr)->sin_port = htons(port);
        inet_pton(AF_INET, ip, &((struct sockaddr_in*)&addr)->sin_addr);
    }
    if (listen_mode) {
        int opt = 1;
        if (setsockopt(wrapper->socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
            SocketError("CreateAsyncSocket", "Failed to set socket reusable");
            goto RELEASE_WRAPPER;
        }
        if (bind(wrapper->socket, &addr, sizeof(addr)) < 0) {
            SocketError("CreateAsyncSocket", "Failed to bind the socket");
            goto RELEASE_WRAPPER;
        }
        if (listen(wrapper->socket, 5) < 0) {
            SocketError("CreateAsyncSocket", "Failed to listen on the socket");
            goto RELEASE_WRAPPER;
        }
    } else {
        if (connect(wrapper->socket, &addr, sizeof(addr)) < 0) {
            SocketError("CreateAsyncSocket", "Failed to connect to the server");
            goto RELEASE_WRAPPER;
        }
    }
    wrapper->ip = (char*)malloc(strlen(ip) + 1);
    if (wrapper->ip == NULL) {
        MemoryError("CreateAsyncSocket", "Failed to allocate memory for ip");
        goto RELEASE_WRAPPER;
    }
    strcpy(wrapper->ip, ip);
    wrapper->port = port;
    wrapper->received_messages = NULL;
    return wrapper;
RELEASE_WRAPPER:
    free(wrapper);
    return NULL;
}

AsyncSocket* CreateAsyncSocketFromSocket(socket_t socket, struct sockaddr addr) {
    AsyncSocket* wrapper = (AsyncSocket*)malloc(sizeof(AsyncSocket));
    if (wrapper == NULL) {
        MemoryError("CreateAsyncSocketFromSocket", "Failed to allocate memory for wrapper");
        return NULL;
    }
    wrapper->socket = socket;
    wrapper->is_listen_socket = 0;
    wrapper->is_ipv6 = addr.sa_family == AF_INET6;
    wrapper->ip = (char*)malloc(wrapper->is_ipv6 ? INET6_ADDRSTRLEN + 1: INET_ADDRSTRLEN + 1);
    if (wrapper->ip == NULL) {
        MemoryError("CreateAsyncSocketFromSocket", "Failed to allocate memory for ip");
        goto RELEASE_WRAPPER;
    }
    wrapper->received_messages = NULL;
    char* ret;
    if (wrapper->is_ipv6) {
        ret = (char*)inet_ntop(AF_INET6, &((struct sockaddr_in6*)&addr)->sin6_addr, wrapper->ip, INET6_ADDRSTRLEN);
        wrapper->port = ntohs(((struct sockaddr_in6*)&addr)->sin6_port);
    } else {
        ret = (char*)inet_ntop(AF_INET, &((struct sockaddr_in*)&addr)->sin_addr, wrapper->ip, INET_ADDRSTRLEN);
        wrapper->port = ntohs(((struct sockaddr_in*)&addr)->sin_port);
    }
    if (ret == NULL) {
        SocketError("CreateAsyncSocketFromSocket", "Failed to convert ip to string");
        goto RELEASE_WRAPPER;
    }
    return wrapper;
RELEASE_WRAPPER:
    free(wrapper);
    return NULL;
}

int ReleaseAsyncSocket(AsyncSocket* async_socket) {
    if (!SOCKET_IS_INVALID(async_socket->socket)) {
        #ifdef _WIN32
        if (closesocket(async_socket->socket) == SOCKET_ERROR) goto SOCKET_ERROR;
        #elif __linux__
        if (close(async_socket->socket) < 0) goto SOCKET_ERROR;
        #endif
    }
    if (async_socket->ip != NULL) free(async_socket->ip);
    free(async_socket);
    LinkNode* cur = async_socket->received_messages;
    while (cur != NULL) {
        Message* message = (Message*)cur->data;
        ReleaseMessage(message);
        cur = cur->next;
    }
    LinkRelease(&async_socket->received_messages);
    return 0;
SOCKET_ERROR:
    SocketError("ReleaseAsyncSocket", "Failed to close the socket");
    return -1;
}