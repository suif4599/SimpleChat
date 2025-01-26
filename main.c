#include <stdio.h>
#include "SimpleChat/WEB/client.h"
#include "user.h"

int on_connect(Server* server, Client* client) {
    printf("New client connected, ip = %s, port = %d\n", client->ip, client->port);
    return 1;
}
int on_disconnect(Server* server, Client* client, int on_error) {
    printf("Client disconnected, on_error = %d\n", on_error);
    if (on_error) {
        return -1;
    }
    return 1;
}
int on_message(Server* server, Client* client) {
    printf("Message received from %s %d: %s\n", client->ip, client->port, client->message);
    return 1;
}

int main(int argc, char *argv[]) {
    // Usage: ./main <name> <port> [<client_ip> <client_port>]
    if (argc != 5 && argc != 3) {
        printf("Usage: %s <name> <port> [<client_ip> <client_port>]\n", argv[0]);
        return 1;
    }
    InitErrorStream();
    Server *server = ServerCreate(argv[1], (uint16_t)atoi(argv[2]), on_connect, NULL, on_message, on_disconnect);
    if (server == NULL) {
        PrintError();
        ReleaseError();
        return 1;
    }
    printf("Server created successfully\n");
    if (argc == 5) {
        if (ConnectServerTo(server, argv[3], (uint16_t)atoi(argv[4])) == -1) {
            PrintError();
            ReleaseError();
            return 1;
        }
    }
    int ret = ServerMainloop(server);
    if (ret < 0) {
        PrintError();
        ReleaseError();
        return 1;
    }
    ServerRelease(server);
    return 0;
}