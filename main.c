#include <stdio.h>
#include <stdlib.h>
#include "simple_tcp_connect.h"

#ifndef _WIN32
#ifndef __linux__
#error "Unsupported OS"
#endif
#endif

int on_connect(struct Client* client)
{
    printf("Client connected, ip: %s\n", client->ip);
    return 0;
}

int on_recv(struct Client* client, char* buffer)
{
    printf("Received data from client %s: %s\n", client->ip, buffer);
    return 0;
}

int on_disconnect(struct Client* client)
{
    printf("Client disconnected, ip: %s\n", client->ip);
    return 0;
}

int main() {
    struct Server* server= create_server("0.0.0.0", 10123, 0);
    if (server == NULL)
    {
        return 1;
    }
    printf("Server created, ip: %s, port: %hu\n", server->ip, server->port);
    start_server(*server, on_connect, on_recv, on_disconnect);
    free(server);
    return 0;
}