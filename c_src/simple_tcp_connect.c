#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "simple_tcp_connect.h"
#include "error_interface.h"

#define ON_DISCONNECT                                                 \
if (on_disconnect != NULL)                                            \
    if (on_disconnect(client) == -1)                                  \
    {                                                                 \
        show_error("DisconnectError", "Error disconnecting client."); \
        return -1;                                                    \
    }                                                                 \
close(client->client_socket);                                         \
free(client);                                                         \
epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->client_socket, NULL);
#define MAX_RECV_LEN 1500


int set_non_blocking(SOCKET socket)
{
    #ifdef _WIN32
    u_long mode = 1;
    if (ioctlsocket(socket, FIONBIO, &mode) != 0) {
        show_error("SocketError", "Failed to set non-blocking mode.");
        return -1;
    }
    #elif __linux__
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1)
    {
        show_error("SocketError", "Failed to get socket flags.");
        return -1;
    }
    return fcntl(socket, F_SETFL, flags | O_NONBLOCK);
    #endif
}

struct Server* create_server(const char *ip, const unsigned short port, const int ipv6)
{
    #ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    #endif
    if (ipv6)
    {
        show_error("NotImplementedError", "IPv6 is not implemented yet.");
        return NULL;
    }
    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == INVALID_SOCKET)
    {
        show_error("SocketError", "Could not create socket.");
        return NULL;
    }
    struct sockaddr_in local = {0};
    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    #ifdef _WIN32
        if (inet_pton(AF_INET, ip, &(local.sin_addr)) != 1)
        {
            show_error("AddressError", "Invalid IP address.");
            return NULL;
        }
    #else
        if (inet_aton(ip, &(local.sin_addr)) == 0)
        {
            show_error("AddressError", "Invalid IP address.");
            return NULL;
        }
    #endif
    if (bind(listen_socket, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR)
    {
        show_error("BindError", "Could not bind socket.");
        return NULL;
    }
    if (listen(listen_socket, 10) == SOCKET_ERROR)
    {
        show_error("ListenError", "Could not listen on socket.");
        return NULL;
    }
    struct Server *server = malloc(sizeof(struct Server));
    if (server == NULL)
    {
        show_error("MemoryError", "Could not allocate memory for server.");
        return NULL;
    }
    set_non_blocking(listen_socket);
    #ifdef __linux__
    int optval = 1;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    #endif
    server->server_socket = listen_socket;
    #ifdef _WIN32
    strcpy_s(server->ip, 48, ip);
    #else
    strcpy(server->ip, ip);
    #endif
    server->port = port;
    server->ipv6 = ipv6;
    return server;
}

int start_server(struct Server server, int (*on_connect)(struct Client*), 
    int (*on_recv)(struct Client*, char*), int (*on_disconnect)(struct Client*))
    // leave callbacks NULL to unset them
    // return -1 to disconnect client and auto call disconnect callback
    // never close client socket in any callback
    // never free anything in any callback
    // if on_disconnect returns -1, it will raise an error
    // on_disconnect is called when client disconnects normally
{
    #ifdef _WIN32
    #elif __linux__
    int epoll_fd = epoll_create(1);
    if (epoll_fd == -1)
    {
        show_error("EpollError", "Could not create epoll instance.");
        return -1;
    }
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = server.server_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server.server_socket, &event) == -1) {
        show_error("EpollError", "Could not add epoll event.");
        return -1;
    }
    struct epoll_event events[128];
    while (1)
    {
        int epoll_ret = epoll_wait(epoll_fd, events, 128, 1000);
        if (epoll_ret == -1)
        {
            show_error("EpollError", "Could not wait for events.");
            return -1;
        }
        else if (epoll_ret == 0) continue;
        struct sockaddr_in addr = {0};
        struct Client* client;
        int size = 0;
        char buffer[MAX_RECV_LEN + 1] = {0};
        for (int i = 0; i < epoll_ret; i++)
        {
            if (events[i].data.fd == server.server_socket)
            {
                int client_socket = accept(server.server_socket, (struct sockaddr*)&addr, &size);
                if (client_socket == -1) continue;
                event.events = EPOLLIN | EPOLLET;
                client = (struct Client*)malloc(sizeof(struct Client));
                client->client_socket = client_socket;
                #ifdef _WIN32
                strcpy_s(client->ip, 48, inet_ntoa(addr.sin_addr));
                #else
                strcpy(client->ip, inet_ntoa(addr.sin_addr));
                #endif
                client->ipv6 = server.ipv6;
                event.data.ptr = (void*)client;
                set_non_blocking(client_socket);
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) == -1)
                {
                    close(client_socket);
                    free(client);
                    continue;
                }
                if (on_connect != NULL)
                    if (on_connect(client) == -1)
                    {
                        ON_DISCONNECT;
                    }
            }
            else
            {
                client = (struct Client*)events[i].data.ptr;
                int bytes = recv(client->client_socket, buffer, MAX_RECV_LEN, 0);
                if (bytes == MAX_RECV_LEN)
                {
                    int len = MAX_RECV_LEN;
                    char* longer_buffer = (char*)malloc(len + MAX_RECV_LEN + 1);
                    if (longer_buffer == NULL)
                    {
                        show_error("MemoryError", "Could not allocate memory for buffer.");
                        return -1;
                    }
                    longer_buffer[len] = '\0';
                    memcpy(longer_buffer, buffer, len);
                    while ((bytes = recv(client->client_socket, (longer_buffer + len), MAX_RECV_LEN, 0)) == MAX_RECV_LEN);
                    {
                        len += MAX_RECV_LEN;
                        longer_buffer = (char*)realloc(longer_buffer, len + MAX_RECV_LEN + 1);
                        if (longer_buffer == NULL)
                        {
                            show_error("MemoryError", "Could not reallocate memory for buffer.");
                            return -1;
                        }
                        longer_buffer[len] = '\0';
                        printf("Reallocated buffer, length = %d, bytes = %d\n", len, bytes);
                    }
                    // if exactly there's no more data to read, bytes == -1
                    if (on_recv != NULL)
                        if (on_recv(client, longer_buffer) == -1)
                        {
                            ON_DISCONNECT;
                            free(longer_buffer);
                            continue;
                        }
                    free(longer_buffer);
                    if (bytes == 0)
                    {
                        ON_DISCONNECT;
                    }
                    continue;
                }
                if (bytes == -1)
                {
                    close(client->client_socket);
                    free(client);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->client_socket, NULL);
                    printf("Error receiving data.\n");
                }
                else if (bytes == 0)
                {
                    ON_DISCONNECT;
                }
                else
                {
                    if (on_recv != NULL)
                        if (on_recv(client, buffer) == -1)
                        {
                            ON_DISCONNECT;
                        }
                }

            }
            
        }
        
    }
    
    #endif
}