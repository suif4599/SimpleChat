#ifndef __CLIENT_H__
#define __CLIENT_H__
#include <sys/socket.h>

#ifdef __linux__
typedef int socket_t;
#elif _WIN32
typedef SOCKET socket_t;
#endif

typedef struct {
    char *name;
    socket_t listen_socket;
} Client;

Client *ClientCreate(const char *name); // Create a new client, return NULL and set global error if failed


void ClientRelease(Client *client);

#endif