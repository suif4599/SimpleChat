#include "client.h"
#include "../../ERROR/error.h"
#include "../../MACRO/macro.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>



static int empty_on_connect(Server* server, Client* client) {
    return 1;
}
static int empty_on_flip(Server* server) {
    return 1;
}
static int empty_on_message(Server* server, Client* client) {
    return 1;
}
static int empty_on_disconnect(Server* server, Client* client, int on_error) {
    return 1;
}
static void delete_index_from_ptr_array(void **arr, int index, int len) {
    if (index < 0 || index >= len) return;
    for (int i = index; i < len - 1; i++)
        arr[i] = arr[i + 1];
}


#ifdef __linux__
static int non_block(int fd) { // Set the socket to non-blocking mode, return 0 if success, otherwise return -1
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        EpollError("non_block", "Failed to get flags");
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        EpollError("non_block", "Failed to set flags");
        return -1;
    }
    return 0;
}
#endif

Server *ServerCreate(const char *name, uint16_t port, 
    int(*on_connect)(struct ServerType* server, Client* client),
    int(*on_flip)(struct ServerType* server),
    int(*on_message)(struct ServerType* server,  Client* client),
    int(*on_disconnect)(struct ServerType* server, Client* client, int on_error)) {
        #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            SystemError("ServerCreate", "Failed to start up WSA");
            return NULL;
        }
        #endif
        Server *server = malloc(sizeof(Server));
        if (server == NULL) {
            MemoryError("ServerCreate", "Failed to allocate memory for server");
            return NULL;
        }
        STR_ASSIGN_TO_SCRATCH(server->name, name, NULL);
        // initialize the listen socket
        server->listen_socket = socket(AF_INET, SOCK_STREAM, 0);
        #ifdef _WIN32
        if (server->listen_socket == INVALID_SOCKET)
        #elif __linux__
        if (server->listen_socket < 0)
        #endif
        {
            SocketError("ServerCreate", "Failed to create a socket");
            return NULL;
        }
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr("0.0.0.0");
        int opt = 1;
        if (setsockopt(server->listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
            SocketError("ServerCreate", "Failed to set socket reusable");
            return NULL;
        }
        if (bind(server->listen_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            SocketError("ServerCreate", "Failed to bind the socket");
            return NULL;
        }
        if (listen(server->listen_socket, 128) < 0) {
            SocketError("ServerCreate", "Failed to listen on the socket");
            return NULL;
        }
        #ifdef __linux__
        if (non_block(server->listen_socket) < 0) {
            RepeatedError("ServerCreate");
            return NULL;
        }
        #endif
        if (on_connect == NULL) server->on_connect = empty_on_connect;
        else server->on_connect = on_connect;
        if (on_flip == NULL) server->on_flip = empty_on_flip;
        else server->on_flip = on_flip;
        if (on_message == NULL) server->on_message = empty_on_message;
        else server->on_message = on_message;
        if (on_disconnect == NULL) server->on_disconnect = empty_on_disconnect;
        else server->on_disconnect = on_disconnect;
        return server;
    }

void ServerRelease(Server *server) {
    free(server->name);
    free(server);
}

#ifdef __linux__
int ServerMainloop(Server *server) {
    int epoll_fd = epoll_create(1);
    if (epoll_fd < 0) {
        EpollError("ServerMainloop", "Failed to create epoll fd");
        return -1;
    }
    struct epoll_event ev;
    ev.events = EPOLLET | EPOLLIN;
    ev.data.ptr = (void*)server;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server->listen_socket, &ev) < 0) { // Note: ev will be copied
        EpollError("ServerMainloop", "Failed to add listen socket to epoll");
        return -1;
    }
    struct epoll_event events[128];
    int nfds, func_ret, ret_len;
    while (1) {
        nfds = epoll_wait(epoll_fd, events, 128, 50);
        if (RefreshErrorStream() < 0) {
            RepeatedError("ServerMainloop");
            return -1;
        }
        if (nfds < 0) {
            EpollError("ServerMainloop", "Failed to wait for events");
            return -1;
        }
        if (nfds > 0) {
            for(int i = 0; i < nfds; i++) {
                if (events[i].data.ptr == (void*)server) { // listen socket have a new connection
                    struct sockaddr_in addr;
                    socklen_t addr_len = sizeof(addr);
                    int client_socket = accept(server->listen_socket, (struct sockaddr*)&addr, &addr_len);
                    if (client_socket < 0) {
                        continue;
                    }
                    if (non_block(client_socket) < 0) {
                        close(client_socket);
                        continue;
                    }
                    memset(&ev, 0, sizeof(ev));
                    ev.events = EPOLLET | EPOLLIN;
                    Client *client = malloc(sizeof(Client));
                    if (client == NULL) {
                        close(client_socket);
                        continue;
                    }
                    client->client_socket = client_socket;
                    client->addr = addr;
                    ClientInit(client);
                    ev.data.ptr = (void*)client;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &ev) < 0) {
                        close(client_socket);
                        free(client);
                        continue;
                    }
                    func_ret = server->on_connect(server, client);
                    if (func_ret < 0) {
                        RepeatedError("ServerMainloop");
                        return -1;
                    }
                    if (func_ret == 0) return 0;
                } else { // client send some message
                    Client *client = (Client*)events[i].data.ptr;
                    char *buffer = malloc(CLIENT_BUFFER_SIZE);
                    int buffer_len = CLIENT_BUFFER_SIZE;
                    ret_len = recv(client->client_socket, buffer, CLIENT_BUFFER_SIZE, 0);
                    if (ret_len <= 0) { // -1: error, 0: client disconnected
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->client_socket, NULL) < 0) {
                            EpollError("ServerMainloop", "Failed to delete client socket from epoll");
                            return -1;
                        }
                        close(client->client_socket);
                        func_ret = server->on_disconnect(server, client, (ret_len < 0) ? 1 : 0);
                        ClientRelease(client);
                        if (func_ret < 0) {
                            RepeatedError("ServerMainloop");
                            return -1;
                        }
                        if (func_ret == 0) return 0;
                    } else { // client send some message
                        while (ret_len == CLIENT_BUFFER_SIZE && buffer[CLIENT_BUFFER_SIZE - 1] != '\0') {
                            buffer = (char*)realloc(buffer, buffer_len += CLIENT_BUFFER_SIZE);
                            if (buffer == NULL) {
                                MemoryError("ServerMainloop", "Failed to reallocate memory for buffer");
                                return -1;
                            }
                            ret_len = recv(client->client_socket, buffer + buffer_len - CLIENT_BUFFER_SIZE, CLIENT_BUFFER_SIZE, 0);
                        }
                        if (ret_len > 0 || GET_LAST_ERROR == 11) {
                            if (client->message != NULL) free(client->message);
                            client->message = buffer;
                            func_ret = server->on_message(server, client);
                            if (func_ret < 0) {
                                RepeatedError("ServerMainloop");
                                return -1;
                            }
                            if (func_ret == 0) return 0;
                            continue;
                        }
                        func_ret = server->on_disconnect(server, client, (ret_len < 0) ? 1 : 0);
                        if (func_ret < 0) {
                            RepeatedError("ServerMainloop");
                            return -1;
                        }
                        if (func_ret == 0) return 0;
                    }
                }
            }
        }
        func_ret = server->on_flip(server);
        if (func_ret < 0) {
            RepeatedError("ServerMainloop");
            return -1;
        }
        if (func_ret == 0) return 0;
    }
    
}
#elif _WIN32
int ServerMainloop(Server *server) {
    WSAEVENT evfd = WSACreateEvent();
    if (evfd == WSA_INVALID_EVENT) {
        EventError("ServerMainloop", "Failed to create WSA event");
        return -1;
    }
    if (WSAEventSelect(server->listen_socket, evfd, FD_ACCEPT) == SOCKET_ERROR) {
        EventError("ServerMainloop", "Failed to select WSA event for listening socket");
        return -1;
    }
    int func_ret, nEvents = 1;
    WSAEVENT events[WSA_MAXIMUM_WAIT_EVENTS];
    Client *clients[WSA_MAXIMUM_WAIT_EVENTS];
    WSANETWORKEVENTS net_event;
    events[0] = evfd;
    while (1) {
        DWORD index = WSAWaitForMultipleEvents(nEvents, events, FALSE, 50, FALSE);
        if (RefreshErrorStream() < 0) {
            RepeatedError("ServerMainloop");
            return -1;
        }
        if (index == WSA_WAIT_FAILED) {
            EventError("ServerMainloop", "Failed to wait for events");
            return -1;
        }
        if (index != WSA_WAIT_TIMEOUT) {
            index -= WSA_WAIT_EVENT_0;
            if (index == 0) { // Events on listen socket
                SOCKET listen_sock = server->listen_socket;
                if (WSAEnumNetworkEvents(listen_sock, evfd, &net_event) == SOCKET_ERROR) {
                    EventError("ServerMainloop", "Failed to get network events");
                    return -1;
                }
                if (net_event.lNetworkEvents & FD_ACCEPT) {
                    // Accept the client
                    if (net_event.iErrorCode[FD_ACCEPT_BIT] != 0) {
                        EventError("ServerMainloop", "Failed to accept the client");
                        return -1;
                    }
                    struct sockaddr_in addr;
                    socklen_t addr_len = sizeof(addr);
                    SOCKET client_socket = accept(listen_sock, (struct sockaddr*)&addr, &addr_len);
                    if (client_socket == INVALID_SOCKET) {
                        continue;
                    }
                    if (nEvents >= WSA_MAXIMUM_WAIT_EVENTS) {
                        if (closesocket(client_socket) == SOCKET_ERROR) {
                            SocketError("ServerMainloop", "Failed to close the client socket");
                            return -1;
                        }
                        continue;
                    }
                    Client *client = malloc(sizeof(Client));
                    if (client == NULL) {
                        MemoryError("ServerMainloop", "Failed to allocate memory for client");
                        return -1;
                    }
                    client->client_socket = client_socket;
                    client->addr = addr;
                    ClientInit(client);
                    WSAEVENT client_event = WSACreateEvent();
                    if (client_event == WSA_INVALID_EVENT) {
                        EventError("ServerMainloop", "Failed to create WSA event for client");
                        return -1;
                    }
                    if (WSAEventSelect(client_socket, client_event, FD_READ | FD_CLOSE) == SOCKET_ERROR) {
                        EventError("ServerMainloop", "Failed to select WSA event for client");
                        return -1;
                    }
                    events[nEvents] = client_event;
                    clients[nEvents] = client;
                    nEvents++;
                    func_ret = server->on_connect(server, client);
                    if (func_ret < 0) {
                        RepeatedError("ServerMainloop");
                        return -1;
                    }
                    if (func_ret == 0) return 0;
                }
            } else {
                if (WSAEnumNetworkEvents(clients[index]->client_socket, events[index], &net_event) == SOCKET_ERROR) {
                    EventError("ServerMainloop", "Failed to get network events");
                    return -1;
                }
                if (net_event.lNetworkEvents & FD_READ) {
                    if (net_event.iErrorCode[FD_READ_BIT] != 0) {
                        EventError("ServerMainloop", "Failed to read the client socket");
                        return -1;
                    }
                    char *buffer = malloc(CLIENT_BUFFER_SIZE);
                    if (buffer == NULL) {
                        MemoryError("ServerMainloop", "Failed to allocate memory for buffer");
                        return -1;
                    }
                    memset(buffer, 0, CLIENT_BUFFER_SIZE);
                    int buffer_len = CLIENT_BUFFER_SIZE;
                    int ret_len = recv(clients[index]->client_socket, buffer, CLIENT_BUFFER_SIZE, 0);
                    if (ret_len <= 0) {
                        if (WSACloseEvent(events[index]) == FALSE) {
                            EventError("ServerMainloop", "Failed to close the client event");
                            return -1;
                        }
                        delete_index_from_ptr_array(&events[0], index, nEvents);
                        if (closesocket(clients[index]->client_socket) == SOCKET_ERROR) {
                            SocketError("ServerMainloop", "Failed to close the client socket");
                            return -1;
                        }
                        func_ret = server->on_disconnect(server, clients[index], (ret_len < 0) ? 1 : 0);
                        ClientRelease(clients[index]);
                        delete_index_from_ptr_array((void *)&clients[0], index, nEvents);
                        nEvents--;
                        if (func_ret < 0) {
                            RepeatedError("ServerMainloop");
                            return -1;
                        }
                        if (func_ret == 0) return 0;
                    } else {
                        while (ret_len == CLIENT_BUFFER_SIZE && buffer[CLIENT_BUFFER_SIZE - 1] != '\0') {
                            buffer = (char*)realloc(buffer, buffer_len += CLIENT_BUFFER_SIZE);
                            if (buffer == NULL) {
                                MemoryError("ServerMainloop", "Failed to reallocate memory for buffer");
                                return -1;
                            }
                            memset(buffer + buffer_len - CLIENT_BUFFER_SIZE, 0, CLIENT_BUFFER_SIZE);
                            ret_len = recv(clients[index]->client_socket, buffer + buffer_len - CLIENT_BUFFER_SIZE, CLIENT_BUFFER_SIZE, 0);
                        }
                        if (ret_len > 0 /*|| GET_LAST_ERROR == 11*/) {
                            if (clients[index]->message != NULL) free(clients[index]->message);
                            clients[index]->message = buffer;
                            func_ret = server->on_message(server, clients[index]);
                            if (func_ret < 0) {
                                RepeatedError("ServerMainloop");
                                return -1;
                            }
                            if (func_ret == 0) return 0;
                            continue;
                        }
                        func_ret = server->on_disconnect(server, clients[index], (ret_len < 0) ? 1 : 0);
                        if (func_ret < 0) {
                            RepeatedError("ServerMainloop");
                            return -1;
                        }
                        if (func_ret == 0) return 0;
                    }
                } else if (net_event.lNetworkEvents & FD_CLOSE) {
                    if (WSACloseEvent(events[index]) == FALSE) {
                        EventError("ServerMainloop", "Failed to close the client event");
                        return -1;
                    }
                    delete_index_from_ptr_array(&events[0], index, nEvents);
                    if (closesocket(clients[index]->client_socket) == SOCKET_ERROR) {
                        SocketError("ServerMainloop", "Failed to close the client socket");
                        return -1;
                    }
                    func_ret = server->on_disconnect(server, clients[index], 0);
                    ClientRelease(clients[index]);
                    delete_index_from_ptr_array((void *)&clients[0], index, nEvents);
                    nEvents--;
                    if (func_ret < 0) {
                        RepeatedError("ServerMainloop");
                        return -1;
                    }
                    if (func_ret == 0) return 0;
                }
            }
        }
        func_ret = server->on_flip(server);
        if (func_ret < 0) {
            RepeatedError("ServerMainloop");
            return -1;
        }
    }
}
#endif


int ClientInit(Client* client) {
    char* buffer = inet_ntoa(client->addr.sin_addr);
    client->ip = malloc(strlen(buffer) + 1);
    if (client->ip == NULL) {
        MemoryError("ClientInit", "Failed to allocate memory for ip");
        return -1;
    }
    strcpy(client->ip, buffer);
    client->port = ntohs(client->addr.sin_port);
    client->message = NULL;
    return 0;
}

void ClientRelease(Client* client) {
    if (client->ip != NULL) free(client->ip);
    if (client->message != NULL) free(client->message);
    free(client);
}