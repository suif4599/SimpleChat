#include "asyncio.h"


AsyncSocket* CreateAsyncSocket(const char* ip, uint16_t port, 
                               int send_time_out, int recv_time_out, // in milliseconds
                               int listen_mode, int receive_mode, int use_ipv6) {
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
    if (listen_mode) { // port reuse and bind
        int opt = 1;
        if (setsockopt(wrapper->socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
            SocketError("CreateAsyncSocket", "Failed to set socket reusable");
            goto CLOSE_SOCKET;
        }
        if (bind(wrapper->socket, &addr, sizeof(addr)) < 0) {
            SocketError("CreateAsyncSocket", "Failed to bind the socket");
            goto CLOSE_SOCKET;
        }
        if (listen(wrapper->socket, 5) < 0) {
            SocketError("CreateAsyncSocket", "Failed to listen on the socket");
            goto CLOSE_SOCKET;
        }
    } else {
        struct timeval timeout;
        timeout.tv_sec = send_time_out / 1000;
        timeout.tv_usec = (send_time_out % 1000) * 1000;
        if (setsockopt(wrapper->socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
            SocketError("CreateAsyncSocket", "Failed to set send timeout");
            goto CLOSE_SOCKET;
        }
        timeout.tv_sec = recv_time_out / 1000;
        timeout.tv_usec = (recv_time_out % 1000) * 1000;
        if (setsockopt(wrapper->socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
            SocketError("CreateAsyncSocket", "Failed to set receive timeout");
            goto CLOSE_SOCKET;
        }
        if (connect(wrapper->socket, &addr, sizeof(addr)) < 0) {
            SocketError("CreateAsyncSocket", "Failed to connect to the server");
            goto CLOSE_SOCKET;
        }
    }
    wrapper->ip = (char*)malloc(strlen(ip) + 1);
    if (wrapper->ip == NULL) {
        MemoryError("CreateAsyncSocket", "Failed to allocate memory for ip");
        goto CLOSE_SOCKET;
    }
    strcpy(wrapper->ip, ip);
    wrapper->port = port;
    wrapper->received_messages = NULL;
    wrapper->caller_frame = NULL;
    wrapper->buffer = NULL;
    wrapper->result_temp = NULL;
    #ifdef _WIN32
    wrapper->event = WSACreateEvent();
    if (wrapper->event == WSA_INVALID_EVENT) {
        free(wrapper->ip);
        SocketError("CreateAsyncSocket", "Failed to create WSA event");
        goto CLOSE_SOCKET;
    }
    #endif
    return wrapper;
CLOSE_SOCKET:
    #ifdef _WIN32
    int ret = closesocket(wrapper->socket);
    #elif __linux__
    int ret = close(wrapper->socket);
    #endif
    if (ret < 0) {
        SocketError("CreateAsyncSocket", "Failed to close the socket");
    }
RELEASE_WRAPPER:
    free(wrapper);
    return NULL;
}

AsyncSocket* CreateAsyncRecvSocketFromSocket(socket_t socket, struct sockaddr addr) {
    AsyncSocket* wrapper = (AsyncSocket*)malloc(sizeof(AsyncSocket));
    if (wrapper == NULL) {
        MemoryError("CreateAsyncSocketFromSocket", "Failed to allocate memory for wrapper");
        return NULL;
    }
    wrapper->socket = socket;
    wrapper->is_listen_socket = 0;
    wrapper->is_receive_socket = 1;
    wrapper->is_ipv6 = addr.sa_family == AF_INET6;
    wrapper->ip = (char*)malloc(wrapper->is_ipv6 ? INET6_ADDRSTRLEN + 1: INET_ADDRSTRLEN + 1);
    if (wrapper->ip == NULL) {
        MemoryError("CreateAsyncSocketFromSocket", "Failed to allocate memory for ip");
        goto RELEASE_WRAPPER;
    }
    wrapper->received_messages = NULL;
    wrapper->caller_frame = NULL;
    wrapper->buffer = NULL;
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
    wrapper->result_temp = NULL;
    return wrapper;
RELEASE_WRAPPER:
    free(wrapper);
    return NULL;
}

int ReleaseAsyncSocket(AsyncSocket* async_socket) {
    if (!SOCKET_IS_INVALID(async_socket->socket)) {
        #ifdef _WIN32
        if (closesocket(async_socket->socket) == SOCKET_ERROR) goto SOCKET_ERROR_LABEL;
        #elif __linux__
        if (close(async_socket->socket) < 0) goto SOCKET_ERROR_LABEL;
        #endif
    }
    if (async_socket->ip != NULL) free(async_socket->ip);
    free(async_socket);
    LinkNode* cur = async_socket->received_messages;
    while (cur != NULL) {
        free(cur->data);
        cur = cur->next;
    }
    LinkRelease(&async_socket->received_messages);
    return 0;
SOCKET_ERROR_LABEL:
    SocketError("ReleaseAsyncSocket", "Failed to close the socket");
    return -1;
}

int BindAsyncSocket(EventLoop* event_loop, AsyncSocket* async_socket) {
    if (LinkAppend(&event_loop->bound_sockets, async_socket) == -1) {
        RepeatedError("BindAsyncSocket");
        return -1;
    }
    #ifdef _WIN32
    // handle event_loop->nEvents & ->events & ->ev_sockets
    // WSAEventSelect
    long lNetworkEvents;
    if (async_socket->is_listen_socket) {
        lNetworkEvents = FD_ACCEPT;
    } else if (async_socket->is_receive_socket) {
        lNetworkEvents = FD_READ;
    } else {
        lNetworkEvents = FD_WRITE;
    }
    if (WSAEventSelect(async_socket->socket, async_socket->event, lNetworkEvents) == SOCKET_ERROR) {
        LinkDeleteData(&event_loop->bound_sockets, async_socket);
        SocketError("RegisterAsyncSocket", "Failed to register the socket");
        // printf("WSAEventSelect failed with error: %d\n", WSAGetLastError());
        return -1;
    }
    event_loop->events[event_loop->nEvents] = async_socket->event;
    event_loop->ev_sockets[event_loop->nEvents] = async_socket;
    event_loop->nEvents++;
    #elif __linux__
    struct epoll_event ev;
    if (async_socket->is_listen_socket) {
        ev.events = EPOLLIN | EPOLLET;
    } else if (async_socket->is_receive_socket){
        ev.events = EPOLLIN; // LT mode
    } else {
        ev.events = EPOLLOUT | EPOLLET;
    }
    ev.data.ptr = async_socket;
    if (epoll_ctl(event_loop->epoll_fd, EPOLL_CTL_ADD, async_socket->socket, &ev) < 0) {
        LinkDeleteData(&event_loop->bound_sockets, async_socket);
        EpollError("RegisterAsyncSocket", "Failed to register the socket");
        return -1;
    }
    #endif
    return 0;
}

int UnbindAsyncSocket(EventLoop* event_loop, AsyncSocket* async_socket) {
    if (LinkDeleteData(&event_loop->bound_sockets, async_socket) == -1) {
        return 0; // Not found
    }
    #ifdef _WIN32
    if (WSAEventSelect(async_socket->socket, async_socket->event, 0) == SOCKET_ERROR) {
        LinkDeleteData(&event_loop->bound_sockets, async_socket);
        EpollError("UnbindAsyncSocket", "Failed to unbind the socket");
        return -1;
    }
    event_loop->nEvents--;
    for (int i = event_loop->nEvents; i >= 0; i--) {
        if (event_loop->ev_sockets[i] == async_socket) {
            for (int j = i; j < event_loop->nEvents; j++) {
                event_loop->events[j] = event_loop->events[j + 1];
                event_loop->ev_sockets[j] = event_loop->ev_sockets[j + 1];
            }
            break;
        }
    }
    #elif __linux__
    if (epoll_ctl(event_loop->epoll_fd, EPOLL_CTL_DEL, async_socket->socket, NULL) < 0) {
        EpollError("UnbindAsyncSocket", "Failed to unbind the socket");
        return -1;
    }
    #endif
    return 0;
}

int DisconnectAsyncSocket(EventLoop* event_loop, AsyncSocket* async_socket) {
    if (UnbindAsyncSocket(event_loop, async_socket) < 0) {
        RepeatedError("DisconnectAsyncSocket");
        return -1;
    }
    if (ReleaseAsyncSocket(async_socket) < 0) {
        RepeatedError("DisconnectAsyncSocket");
        return -1;
    }
    return 0;
}