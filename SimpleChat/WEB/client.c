#include "client.h"
#include "../../ERROR/error.h"
#include "../../MACRO/macro.h"
#include <string.h>




Client *ClientCreate(const char *name) {
    Client *client = malloc(sizeof(Client));
    if (client == NULL) {
        MemoryError("ClientCreate", "Failed to allocate memory for client");
        return NULL;
    }
    STR_ASSIGN_TO_SCRATCH(client->name, name, NULL);
    // initialize the listen socket
    client->listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    #ifdef _WIN32
    if (client->listen_socket == INVALID_SOCKET)
    #elif __linux__
    if (client->listen_socket < 0)
    #endif
    {
        SocketError("ClientCreate", "Failed to create a socket");
        return NULL;
    }
    
    return client;
}