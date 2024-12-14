#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "simple_tcp_connect.h"
#include "error_interface.h"


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

void start_server(struct Server server)
{
    #ifdef _WIN32
    #elif __linux__
    int epoll_fd = epoll_create(1);
    if (epoll_fd == -1)
    {
        show_error("EpollError", "Could not create epoll instance.");
        return;
    }
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = server.server_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server.server_socket, &event) == -1) {
        show_error("EpollError", "Could not add epoll event.");
    }
    struct epoll_event events[128];
    while (1)
    {
        int epoll_ret = epoll_wait(epoll_fd, events, 128, 1);
        if (epoll_ret == -1)
        {
            show_error("EpollError", "Could not wait for events.");
            return;
        }
        else if (epoll_ret == 0) continue;
        struct sockaddr_in addr = {0};
        int size = 0;
        for (int i = 0; i < epoll_ret; i++)
        {
            if (events[i].data.fd == server.server_socket)
            {
                int client_socket = accept(server.server_socket, (struct sockaddr*)&addr, &size);
                if (client_socket == -1) continue;
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = server.server_socket;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) == -1)
                {
                    close(client_socket);
                    continue;
                }
            }
            else
            {
                
            }
            
        }
        
    }
    
    #endif
}