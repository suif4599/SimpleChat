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
    if(ipv6)
    {
        show_error("NotImplementedError", "IPv6 is not implemented yet.");
        return NULL;
    }
    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_socket == INVALID_SOCKET)
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
    if(bind(listen_socket, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR)
    {
        show_error("BindError", "Could not bind socket.");
        return NULL;
    }
    if(listen(listen_socket, 10) == SOCKET_ERROR)
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