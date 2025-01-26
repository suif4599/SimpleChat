#ifndef __CLIENT_H__
#define __CLIENT_H__
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdint.h>
#elif __linux__
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#endif
#include "../UI/ui.h"

#ifdef __linux__
typedef int socket_t;
#define SOCKET_IS_INVALID(s) ((s) < 0)
#elif _WIN32
typedef SOCKET socket_t;
#define SOCKET_IS_INVALID(s) ((s) == INVALID_SOCKET)
#endif

#define CLIENT_BUFFER_SIZE 300
#define STDIN_BUFFER_SIZE 5


typedef struct {
    socket_t client_socket;
    struct sockaddr_in addr;
    char* ip;
    uint16_t port;
    char* message;
    socket_t target_socket; // the socket that the client want to send message to
} Client;

typedef struct LinkNodeType{
    void *data;
    struct LinkNodeType *next;
} LinkNode;

typedef struct ServerType {
    char *name;
    uint16_t port;
    socket_t listen_socket;
    LinkNode *clients;
    socket_t temp_socket; // used for temporary socket
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
void ServerRelease(Server *server); // Release the server, it won't close the socket, it won't raise any error
int ServerMainloop(Server *server); // Main loop of the server, return 0 if quiting normally, otherwise return -1
int ConnectServerTo(Server *server, const char *ip, uint16_t port); 
    // Connect the server to another server, return -1 if failed
    // If server->listen_socket is INVALID_SOCKET or -1, it will automatically initialize a new socket and won't send port number to the target server

int ClientInit(Client* client, socket_t target_socket, uint16_t server_port); 
    // Initialize the client, make target_socket -1 or INVALID_SOCKET to automatically initialize, return -1 if failed
void ClientRelease(Client* client); // Release the client, it won't close the socket, it won't raise any error

void LinkAppend(LinkNode **head, void *data); // Append a new node to the end of the list
void LinkDelete(LinkNode **head, void *data); // Delete a node from the list, it won't free data
void LinkRelease(LinkNode **head); // Release the list, it won't release the data, it won't raise any error

#endif