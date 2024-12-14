#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#ifdef __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

#ifndef SIMPLE_TCP_CONNECT_H
#define SIMPLE_TCP_CONNECT_H

struct Server
{
    SOCKET server_socket;
    char ip[48];
    unsigned short port;
    int ipv6;
};

struct Server* create_server(const char *ip, const unsigned short port, const int ipv6);

#endif // SIMPLE_TCP_CONNECT_H;