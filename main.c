#include <stdio.h>
#include <stdlib.h>
#include "simple_tcp_connect.h"


int main() {
    struct Server* server= create_server("0.0.0.0", 10123, 0);
    if(server == NULL)
    {
        return 1;
    }
    printf("Server created, ip: %s, port: %hu\n", server->ip, server->port);
    free(server);
    return 0;
}