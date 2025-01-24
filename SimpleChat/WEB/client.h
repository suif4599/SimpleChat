#ifndef __CLIENT_H__
#define __CLIENT_H__
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#elif __linux__
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#endif

#ifdef __linux__
typedef int socket_t;
#elif _WIN32
typedef SOCKET socket_t;
#endif

#define CLIENT_BUFFER_SIZE 3


typedef struct {
    socket_t client_socket;
    struct sockaddr_in addr;
    char* ip;
    uint16_t port;
    char* message;
} Client;


typedef struct ServerType {
    char *name;
    socket_t listen_socket;
    // Return 0 to stop the loop, 1 to continue the loop, -1 to abort the loop with a RepeatedError
    int(*on_connect)(struct ServerType* server, Client* client); // called when a new client is connected successfully
    int(*on_flip)(struct ServerType* server);
    int(*on_message)(struct ServerType* server,  Client* client);
    int(*on_disconnect)(struct ServerType* server, Client* client, int on_error); // if the client doesn't exit normally, on_error would be 1, otherwise 0
} Server;


Server *ServerCreate(const char *name, uint16_t port, 
    int(*on_connect)(struct ServerType* server, Client* client),
    int(*on_flip)(struct ServerType* server),
    int(*on_message)(struct ServerType* server,  Client* client),
    int(*on_disconnect)(struct ServerType* server, Client* client, int on_error)); // Create a new server, return NULL if failed
void ServerRelease(Server *server);
int ServerMainloop(Server *server); // Main loop of the server, return 0 if quiting normally, otherwise return -1

int ClientInit(Client* client); // Initialize the client, return -1 if failed
void ClientRelease(Client* client); // Release the client

#endif