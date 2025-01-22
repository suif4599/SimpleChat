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
    
    return client;
}