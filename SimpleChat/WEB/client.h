#ifndef __CLIENT_H__
#define __CLIENT_H__

typedef struct {
    char *name;
    char* ip;
} Client;

Client *ClientCreate(const char *name); // Create a new client, return NULL and set global error if failed


void ClientRelease(Client *client);

#endif