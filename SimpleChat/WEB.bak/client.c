#include "client.h"
#include "../../ERROR/error.h"
#include "../../MACRO/macro.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


static LinkNode *COMMAND_LIST = NULL;
static LinkNode *MSG_LIST = NULL;
static LinkNode *BUSY_CLIENT_LIST = NULL;
struct Msg {
    Server *server;
    Client *client;
    const char *msg;
};
static LinkNode *ROGER_SOCKET_LIST = NULL;
struct RogerSocket {
    char *name;
    socket_t socket;
};
static uint16_t target_port_temp = 0;
#define COMMAND_TAG "command756328"
#define MSG_HEADER "header952642"
#define MSG_RECEIVED_TAG "<" COMMAND_TAG ">report_msg_received</" COMMAND_TAG ">"

static int empty_on_connect(Server* server, Client* client) {
    return 1;
}
static int empty_on_flip(Server* server) {
    return 1;
}
static int empty_on_message(Server* server, Client* client) {
    return 1;
}
static int empty_on_disconnect(Server* server, Client* client, int on_error) {
    return 1;
}
static void delete_index_from_ptr_array(void **arr, int index, int len) {
    IntoFunction("delete_index_from_ptr_array");
    if (index < 0 || index >= len) return;
    for (int i = index; i < len - 1; i++)
        arr[i] = arr[i + 1];
}

static int client_is_busy(Client *client) {
    LinkNode *cur = BUSY_CLIENT_LIST;
    while (cur != NULL) {
        if (cur->data == client) return 1;
        cur = cur->next;
    }
    return 0;
}

static int __send_msg() {
    LinkNode *cur = MSG_LIST;
    while (cur != NULL) {
        struct Msg *msg = (struct Msg*)cur->data;
        if (!client_is_busy(msg->client)) {
            int ret = send(msg->client->target_socket, msg->msg, (int)strlen(msg->msg), 0);
            if (ret < 0) {
                SocketError("__send_msg", "Failed to send message");
                return -1;
            }
            printf("\033[33mMsg sent\033[0m: %s to (%s,%d) is sent\n", msg->msg, msg->client->ip, msg->client->port);
            free((void *)msg->msg);
            LinkDeleteData(&MSG_LIST, cur->data);
            LinkAppend(&BUSY_CLIENT_LIST, msg->client);
        } 
        else {
            printf("\033[33mMsg not sent\033[0m %s to (%s,%d) is not sent\n", msg->msg, msg->client->ip, msg->client->port);
        }
        cur = cur->next;
    }
    return 0;
}

static int send_msg(Server *server, Client *client, const char *msg) {
    IntoFunction("send_msg");
    struct Msg *_msg = malloc(sizeof(struct Msg));
    if (_msg == NULL) {
        MemoryError("send_msg", "Failed to allocate memory for msg");
        return -1;
    }
    _msg->server = server;
    _msg->client = client;
    _msg->msg = malloc(strlen(msg) + 2 * strlen(MSG_HEADER) + 6);
    if (_msg->msg == NULL) {
        MemoryError("send_msg", "Failed to allocate memory for msg");
        free(_msg);
        return -1;
    }
    sprintf((char*)_msg->msg, "<%s>%s</%s>", MSG_HEADER, msg, MSG_HEADER);
    LinkAppend(&MSG_LIST, _msg);
    printf("\033[33mMessage added to the list\033[0m: from = (%s,%d), to = (%s,%d), msg = (%s)\n", server->ip, server->port, client->ip, client->port, msg);
    return 0;
}

static int send_msg_to_all_clients(Server *server, const char *msg) {
    IntoFunction("send_msg_to_all_clients");
    LinkNode *cur = server->clients;
    while (cur != NULL) {
        Client *client = (Client*)cur->data;
        if (send_msg(server, client, msg) < 0) {
            RepeatedError("send_msg_to_all_clients");
            return -1;
        }
        cur = cur->next;
    }
    return 0;
}

int add_command(const char *name, CommandHandler handler) {
    IntoFunction("add_command");
    Command *command = malloc(sizeof(Command));
    if (command == NULL) {
        MemoryError("add_command", "Failed to allocate memory for command");
        return -1;
    }
    STR_ASSIGN_TO_SCRATCH(command->name, name, -1);
    command->handler = handler;
    LinkAppend(&COMMAND_LIST, command);
    return 0;
}

static CommandHandler get_command_handler(const char *name) {
    IntoFunction("get_command_handler");
    LinkNode *cur = COMMAND_LIST;
    while (cur != NULL) {
        Command *command = (Command*)cur->data;
        if (strcmp(command->name, name) == 0) {
            return command->handler;
        }
        cur = cur->next;
    }
    NameError("get_command_handler", "Command not found");
    return NULL;
}

int send_command(Server *server, Client* client, const char *name, const char *arg) {
    IntoFunction("send_command");
    if (get_command_handler(name) == NULL) {
        CommandError("send_command", "Command not found");
        return -1;
    }
    char *msg = malloc(strlen(COMMAND_TAG) * 2 + strlen(name) + strlen(arg) + 6);
    if (msg == NULL) {
        MemoryError("send_command", "Failed to allocate memory for msg");
        return -1;
    }
    sprintf(msg, "<%s>%s</%s>%s", COMMAND_TAG, name, COMMAND_TAG, arg);
    int ret = send_msg(server, client, msg);
    free(msg);
    if (ret < 0) {
        RepeatedError("send_command");
        return -1;
    }
    return 0;
}

static int msg_received(Server *server, Client* client) { // Terminal call this to report the message received
    IntoFunction("msg_received");
    static const char* name = "report_msg_received";
    static const char* arg = "OK";
    char *msg = malloc(strlen(COMMAND_TAG) * 2 + strlen(MSG_HEADER) * 2 + strlen(name) + strlen(arg) + 11);
    if (msg == NULL) {
        MemoryError("send_command", "Failed to allocate memory for msg");
        return -1;
    }
    sprintf(msg, "<%s><%s>%s</%s>%s</%s>", MSG_HEADER, COMMAND_TAG, name, COMMAND_TAG, arg, MSG_HEADER);
    int ret = send(client->target_socket, msg, (int)strlen(msg), 0);
    free(msg);
    if (ret < 0) {
        SocketError("msg_received", "Failed to send message");
        return -1;
    }
    return 0;
}

int send_command_to_all_clients(Server *server, const char *name, const char *arg) {
    IntoFunction("send_command_to_all_clients");
    if (get_command_handler(name) == NULL) {
        CommandError("send_command_to_all_clients", "Command not found");
        return -1;
    }
    char *msg = malloc(strlen(COMMAND_TAG) * 2 + strlen(name) + strlen(arg) + 6);
    if (msg == NULL) {
        MemoryError("send_command_to_all_clients", "Failed to allocate memory for msg");
        return -1;
    }
    sprintf(msg, "<%s>%s</%s>%s", COMMAND_TAG, name, COMMAND_TAG, arg);
    int ret = send_msg_to_all_clients(server, msg);
    free(msg);
    if (ret < 0) {
        RepeatedError("send_command_to_all_clients");
        return -1;
    }
    return 0;
}

static int check_command_and_call(const char* msg, Server *server, Client *client) { 
    // Check if a msg is a command, if so, call the handler and return 0, otherwise return 1, if failed, return -1
    IntoFunction("check_command_and_call");
    if (strncmp(msg, "<" COMMAND_TAG ">", strlen(COMMAND_TAG) + 2) != 0) return 1;
    int i = strlen(COMMAND_TAG) + 2;
    while (strncmp(msg + i, "</" COMMAND_TAG ">", strlen(COMMAND_TAG) + 3) != 0 && msg[i] != '\0') i++;
    if (msg[i] == '\0') return 1;
    if (strncmp(msg + i, "</" COMMAND_TAG ">", strlen(COMMAND_TAG) + 3) != 0) return 1;
    char *name = malloc(i - strlen(COMMAND_TAG) - 1);
    if (name == NULL) {
        MemoryError("check_command_and_call", "Failed to allocate memory for name");
        return -1;
    }
    strncpy(name, msg + strlen(COMMAND_TAG) + 2, i - strlen(COMMAND_TAG) - 2);
    name[i - strlen(COMMAND_TAG) - 2] = '\0';
    CommandHandler handler = get_command_handler(name);
    printf("Calling command (%s) with server=(%s:%d), client=(%s:%d), argument=(%s)\n", 
        name, server->ip, server->port, client->ip, client->port, msg + i + strlen(COMMAND_TAG) + 3);
    free(name);
    if (handler == NULL) {
        RepeatedError("check_command_and_call");
        return -1;
    }
    int ret = handler(server, client, msg + i + strlen(COMMAND_TAG) + 3);
    if (ret < 0) {
        RepeatedError("check_command_and_call");
        return -1;
    }
    return 0;
}

static int parse_msg(char **msg, char **result) {
    // if the msg is complete, set <result> and remove the message from <msg> return 0
    // if the msg is incomplete, do nothing and return 1
    // if failed, do nothing and return -1
    IntoFunction("parse_msg");
    int len = strlen(*msg);
    if (len < strlen(MSG_HEADER) + 5) return 1;
    if (strncmp(*msg, "<" MSG_HEADER ">", strlen(MSG_HEADER) + 2) != 0) {
        BaseError("parse_msg", "Invalid message header");
        return -1;
    }
    int i = strlen(MSG_HEADER) + 2;
    while (strncmp(*msg + i, "</" MSG_HEADER ">", strlen(MSG_HEADER) + 3) != 0 && (*msg)[i] != '\0') i++;
    if ((*msg)[i] == '\0') return 1;
    if (strncmp(*msg + i, "</" MSG_HEADER ">", strlen(MSG_HEADER) + 3) != 0) return 1;
    *result = malloc(i - strlen(MSG_HEADER) - 1);
    if (*result == NULL) {
        MemoryError("parse_msg", "Failed to allocate memory for result");
        return -1;
    }
    strncpy(*result, *msg + strlen(MSG_HEADER) + 2, i - strlen(MSG_HEADER) - 2);
    (*result)[i - strlen(MSG_HEADER) - 2] = '\0';
    char *temp = malloc(len - i - strlen(MSG_HEADER) - 1);
    if (temp == NULL) {
        MemoryError("parse_msg", "Failed to allocate memory for temp");
        free(*result);
        return -1;
    }
    strncpy(temp, *msg + i + strlen(MSG_HEADER) + 3, len - i - strlen(MSG_HEADER) - 3);
    temp[len - i - strlen(MSG_HEADER) - 3] = '\0';
    free(*msg);
    *msg = temp;
    return 0;
}

#ifdef __linux__
static int non_block(int fd) { // Set the socket to non-blocking mode, return 0 if success, otherwise return -1
    IntoFunction("non_block");
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        EpollError("non_block", "Failed to get flags");
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        EpollError("non_block", "Failed to set flags");
        return -1;
    }
    return 0;
}
#endif

static int parse_ip_and_port(const char *str, char **ip, uint16_t *port) {
    // parse the ip and port from the string, it will malloc for *ip, remember to free it
    // return 0 if success, -1 if failed
    IntoFunction("parse_ip_and_port");
    int len = strlen(str);
    for(int i = 0; i < len; i++) {
        if (str[i] == ':') {
            *ip = malloc(i + 1);
            if (*ip == NULL) {
                MemoryError("parse_ip_and_port", "Failed to allocate memory for ip");
                return -1;
            }
            strncpy(*ip, str, i);
            (*ip)[i] = '\0';
            *port = atoi(str + i + 1);
            return 0;
        }
    }
    BaseError("parse_ip_and_port", "Invalid ip and port string");
    return -1;
}


static int set_client_name(Server *_server, Client *client, const char *name) {
    IntoFunction("set_client_name");
    if (client->name != NULL) free(client->name);
    STR_ASSIGN_TO_SCRATCH(client->name, name, -1);

}

static int set_server_ip(Server *server, Client *_client, const char *ip) {
    IntoFunction("set_server_ip");
    if (server->ip != NULL) free(server->ip);
    STR_ASSIGN_TO_SCRATCH(server->ip, ip, -1);
    return 0;
}

static int request_connect_clients_to(Server *server, Client *_client, const char *name) {
    // make every client of server connect to the target name
    IntoFunction("request_connect_clients_to");
    LinkNode *cur = server->clients;
    while (cur != NULL) {
        Client *client = (Client*)cur->data;
        if (client == _client) {
            cur = cur->next;
            continue;
        }
        if (send_command(server, client, "request_connect_to", name) < 0) {
            RepeatedError("request_connect_clients_to");
            return -1;
        }
        cur = cur->next;
    }
}

static int request_connect_to(Server *server, Client *client, const char *name) {
    // make the server connect to the target name
    IntoFunction("request_connect_to");
    char *ip;
    uint16_t port;
    if (parse_ip_and_port(name, &ip, &port) < 0) {
        RepeatedError("request_connect_to");
        free(ip);
        return -1;
    }
    if (ConnectServerTo(server, ip, port, 0) < 0) {
        RepeatedError("request_connect_to");
        free(ip);
        return -1;
    }
    free(ip);
}

static int report_msg_received(Server *server, Client *client, const char *_msg) {
    IntoFunction("report_msg_received");
    LinkNode *cur = BUSY_CLIENT_LIST;
    while (cur != NULL) {
        Client *busy_client = (Client*)cur->data;
        if (busy_client == client) {
            LinkDeleteData(&BUSY_CLIENT_LIST, client);
            break;
        }
        cur = cur->next;
    }
    return 0;
}

Server *ServerCreate(const char *name, uint16_t port, 
    int(*on_connect)(struct ServerType* server, Client* client),
    int(*on_flip)(struct ServerType* server),
    int(*on_message)(struct ServerType* server,  Client* client),
    int(*on_disconnect)(struct ServerType* server, Client* client, int on_error)) {
        IntoFunction("ServerCreate");
        add_command("set_client_name", set_client_name);
        add_command("set_server_ip", set_server_ip);
        add_command("request_connect_clients_to", request_connect_clients_to);
        add_command("request_connect_to", request_connect_to);
        add_command("report_msg_received", report_msg_received);
        #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            SystemError("ServerCreate", "Failed to start up WSA");
            return NULL;
        }
        #endif
        Server *server = malloc(sizeof(Server));
        if (server == NULL) {
            MemoryError("ServerCreate", "Failed to allocate memory for server");
            return NULL;
        }
        STR_ASSIGN_TO_SCRATCH(server->name, name, NULL);
        server->ip = NULL;
        server->port = port;
        server->clients = NULL;
        #ifdef _WIN32
        server->temp_socket = INVALID_SOCKET;
        server->temp_roger_socket = INVALID_SOCKET;
        #elif __linux__
        server->temp_socket = -1;
        server->temp_roger_socket = -1;
        #endif
        // initialize the listen socket
        server->listen_socket = socket(AF_INET, SOCK_STREAM, 0);
        #ifdef _WIN32
        if (server->listen_socket == INVALID_SOCKET)
        #elif __linux__
        if (server->listen_socket < 0)
        #endif
        {
            SocketError("ServerCreate", "Failed to create a socket");
            return NULL;
        }
        server->listen_roger_socket = socket(AF_INET, SOCK_STREAM, 0);
        #ifdef _WIN32
        if (server->listen_roger_socket == INVALID_SOCKET)
        #elif __linux__
        if (server->listen_roger_socket < 0)
        #endif
        {
            SocketError("ServerCreate", "Failed to create a socket");
            return NULL;
        }
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr("0.0.0.0");
        int opt = 1;
        if (setsockopt(server->listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0 ||
            setsockopt(server->listen_roger_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
            SocketError("ServerCreate", "Failed to set socket reusable");
            #ifdef __linux__
            close(server->listen_socket);
            #elif _WIN32
            closesocket(server->listen_socket);
            #endif
            return NULL;
        }
        if (bind(server->listen_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            SocketError("ServerCreate", "Failed to bind the socket");
            return NULL;
        }
        addr.sin_port = htons(port + 1);
        if (bind(server->listen_roger_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            SocketError("ServerCreate", "Failed to bind the socket");
            return NULL;
        }
        if (listen(server->listen_socket, 128) < 0 || listen(server->listen_roger_socket, 128) < 0) {
            SocketError("ServerCreate", "Failed to listen on the socket");
            return NULL;
        }
        #ifdef __linux__
        if (non_block(server->listen_socket) < 0 || non_block(server->listen_roger_socket) < 0) {
            RepeatedError("ServerCreate");
            return NULL;
        }
        #endif
        if (on_connect == NULL) server->on_connect = empty_on_connect;
        else server->on_connect = on_connect;
        if (on_flip == NULL) server->on_flip = empty_on_flip;
        else server->on_flip = on_flip;
        if (on_message == NULL) server->on_message = empty_on_message;
        else server->on_message = on_message;
        if (on_disconnect == NULL) server->on_disconnect = empty_on_disconnect;
        else server->on_disconnect = on_disconnect;
        #ifdef _WIN32
        if (InitializeUserInterface() < 0) {
            RepeatedError("ServerCreate");
            return NULL;
        }
        #endif
        server->temp_recrusion_connect = 0;
        return server;
    }

void ServerRelease(Server *server) {
    IntoFunction("ServerRelease");
    free(server->name);
    free(server->ip);
    free(server);
}

#ifdef __linux__
int ServerMainloop(Server *server) {
    IntoFunction("ServerMainloop");
    char* msg_for_send = NULL;
    int epoll_fd = epoll_create(1);
    if (epoll_fd < 0) {
        EpollError("ServerMainloop", "Failed to create epoll fd");
        return -1;
    }
    struct epoll_event ev;
    ev.events = EPOLLET | EPOLLIN;
    ev.data.ptr = (void*)server;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server->listen_socket, &ev) < 0) { // Note: ev will be copied
        EpollError("ServerMainloop", "Failed to add listen socket to epoll");
        return -1;
    }
    ev.data.ptr = (void*)((char*)server + 4);
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server->listen_roger_socket, &ev) < 0) {
        EpollError("ServerMainloop", "Failed to add listen roger socket to epoll");
        return -1;
    }
    int stdin_fd = fileno(stdin);
    ev.events = EPOLLIN;
    ev.data.ptr = &stdin_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, stdin_fd, &ev) < 0) {
        EpollError("ServerMainloop", "Failed to add stdin to epoll");
        return -1;
    }
    struct epoll_event events[128];
    int nfds, func_ret, ret_len;
    while (1) {
        nfds = epoll_wait(epoll_fd, events, 128, 50);
        if (RefreshErrorStream() < 0) {
            RepeatedError("ServerMainloop");
            return -1;
        }
        if (nfds < 0) {
            EpollError("ServerMainloop", "Failed to wait for events");
            return -1;
        }
        if (nfds > 0) {
            for(int i = 0; i < nfds; i++) {
                if (events[i].data.ptr == &stdin_fd) { // stdin have some input
                    char *buffer = readLine(STDIN_BUFFER_SIZE);
                    if (buffer == NULL) {
                        RepeatedError("ServerMainloop");
                        return -1;
                    }
                    if (msg_for_send != NULL) {
                        free(msg_for_send);
                    }
                    msg_for_send = buffer;
                } else if (events[i].data.ptr == (void*)((char*)server + 4)) { // listen roger socket have a new connection
                    int roger_socket = accept(server->listen_roger_socket, NULL, NULL);
                    if (roger_socket < 0) {
                        continue;
                    }
                    char *buffer = malloc(MAX_NAME_LENGTH);
                    if (buffer == NULL) {
                        MemoryError("ServerMainloop", "Failed to allocate memory for buffer");
                        return -1;
                    }
                    int ret = recv(roger_socket, buffer, MAX_NAME_LENGTH, 0);
                    if (ret <= 0) {
                        close(roger_socket);
                        continue;
                    }
                    buffer[ret] = '\0';
                    struct RogerSocket *roger = malloc(sizeof(struct RogerSocket));
                    if (roger == NULL) {
                        MemoryError("ServerMainloop", "Failed to allocate memory for roger");
                        return -1;
                    }
                    roger->name = buffer;
                    roger->socket = roger_socket;
                    LinkAppend(&ROGER_SOCKET_LIST, roger);
                } else if (events[i].data.ptr == (void*)server) { // listen socket have a new connection
                    struct sockaddr_in addr;
                    socklen_t addr_len = sizeof(addr);
                    int client_socket = accept(server->listen_socket, (struct sockaddr*)&addr, &addr_len);
                    if (client_socket < 0) {
                        continue;
                    }
                    ev.events = EPOLLET | EPOLLIN;
                    char *buffer = malloc(MAX_NAME_LENGTH + 1);
                    int ret = recv(client_socket, buffer, MAX_NAME_LENGTH, 0);
                    if (ret <= 0) {
                        close(client_socket);
                        continue;
                    }
                    buffer[ret] = '\0';
                    Client *client = malloc(sizeof(Client));
                    if (client == NULL) {
                        close(client_socket);
                        continue;
                    }
                    client->name = buffer;
                    LinkNode *cur = ROGER_SOCKET_LIST;
                    int flag = 0;
                    while (cur != NULL) {
                        struct RogerSocket *roger = (struct RogerSocket*)cur->data;
                        if (strcmp(roger->name, client->name) == 0) {
                            client->roger_socket = roger->socket;
                            free(roger->name);
                            LinkDeleteData(&ROGER_SOCKET_LIST, roger);
                            flag = 1;
                        }
                        cur = cur->next;
                    }
                    if (flag == 0) {
                        RuntimeError("set_client_name", "Roger socket not found");
                        return -1;
                    zai}
                    client->client_socket = client_socket;
                    client->addr = addr;
                    if (ClientInit(client, server->name, server->temp_socket, server->port, server->temp_roger_socket) == -1) {
                        close(client_socket);
                        free(client);
                        RepeatedError("ServerMainloop");
                        return -1;
                    }
                    server->temp_socket = -1;
                    LinkAppend(&server->clients, client);
                    ev.data.ptr = (void*)client;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &ev) < 0) {
                        LinkDeleteData(&server->clients, client);
                        close(client_socket);
                        close(client->target_socket);
                        free(client);
                        continue;
                    }
                    // if (send_command(server, client, "set_client_name", server->name) < 0) {
                    //     LinkDelete(&server->clients, client);
                    //     close(client_socket);
                    //     close(client->target_socket);
                    //     free(client);
                    //     continue;
                    // }
                    if (send_command(server, client, "set_server_ip", client->ip) < 0) {
                        LinkDeleteData(&server->clients, client);
                        close(client_socket);
                        close(client->target_socket);
                        free(client);
                        continue;
                    }
                    if (server->temp_recrusion_connect) {
                        if (RecursionConnect(server) < 0) {
                            LinkDeleteData(&server->clients, client);
                            close(client_socket);
                            close(client->target_socket);
                            free(client);
                            continue;
                        }
                    }
                    func_ret = server->on_connect(server, client);
                    if (func_ret < 0) {
                        RepeatedError("ServerMainloop");
                        return -1;
                    }
                    if (func_ret == 0) return 0;
                } else { // client send some message
                    Client *client = (Client*)events[i].data.ptr;
                    char *buffer = malloc(CLIENT_BUFFER_SIZE);
                    memset(buffer, 0, CLIENT_BUFFER_SIZE);
                    int buffer_len = CLIENT_BUFFER_SIZE;
                    ret_len = recv(client->client_socket, buffer, CLIENT_BUFFER_SIZE, 0);
                    if (ret_len <= 0) { // -1: error, 0: client disconnected
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->client_socket, NULL) < 0) {
                            EpollError("ServerMainloop", "Failed to delete client socket from epoll");
                            return -1;
                        }
                        LinkDeleteData(&server->clients, client);
                        close(client->client_socket);
                        close(client->target_socket);
                        func_ret = server->on_disconnect(server, client, (ret_len < 0) ? 1 : 0);
                        ClientRelease(client);
                        if (func_ret < 0) {
                            RepeatedError("ServerMainloop");
                            return -1;
                        }
                        if (func_ret == 0) return 0;
                    } else { // client send some message
                        while (ret_len == CLIENT_BUFFER_SIZE && buffer[CLIENT_BUFFER_SIZE - 1] != '\0') {
                            buffer = (char*)realloc(buffer, buffer_len += CLIENT_BUFFER_SIZE);
                            if (buffer == NULL) {
                                MemoryError("ServerMainloop", "Failed to reallocate memory for buffer");
                                return -1;
                            }
                            memset(buffer + buffer_len - CLIENT_BUFFER_SIZE, 0, CLIENT_BUFFER_SIZE);
                            ret_len = recv(client->client_socket, buffer + buffer_len - CLIENT_BUFFER_SIZE, CLIENT_BUFFER_SIZE, 0);
                        }
                        if (ret_len > 0 || GET_LAST_ERROR == 11) {
                            if (client->message == NULL) {
                                client->message = buffer;
                                buffer = NULL;
                            } else {
                                char *temp = malloc(strlen(client->message) + strlen(buffer) + 1);
                                if (temp == NULL) {
                                    MemoryError("ServerMainloop", "Failed to allocate memory for temp");
                                    return -1;
                                }
                                strcpy(temp, client->message);
                                strcat(temp, buffer);
                                free(client->message);
                                free(buffer);
                                client->message = temp;
                            }
                            func_ret = parse_msg(&client->message, &buffer);
                            if (func_ret < 0) {
                                RepeatedError("ServerMainloop");
                                return -1;
                            }
                            if (func_ret == 1) continue;
                            if (strncmp(buffer, MSG_RECEIVED_TAG, strlen(MSG_RECEIVED_TAG)) != 0) {
                                if (msg_received(server, client) < 0) {
                                    RepeatedError("ServerMainloop");
                                    return -1;
                                }
                            }
                            func_ret = check_command_and_call(buffer, server, client);
                            if (func_ret < 0) {
                                RepeatedError("ServerMainloop");
                                return -1;
                            }
                            if (func_ret == 0) continue;
                            char *temp = client->message;
                            client->message = buffer;
                            func_ret = server->on_message(server, client);
                            client->message = temp;
                            free(buffer);
                            if (func_ret < 0) {
                                RepeatedError("ServerMainloop");
                                return -1;
                            }
                            if (func_ret == 0) return 0;
                            continue;
                        }
                        LinkDeleteData(&server->clients, client);
                        close(client->client_socket);
                        close(client->target_socket);
                        func_ret = server->on_disconnect(server, client, (ret_len < 0) ? 1 : 0);
                        if (func_ret < 0) {
                            RepeatedError("ServerMainloop");
                            return -1;
                        }
                        if (func_ret == 0) return 0;
                    }
                }
            }
        }
        if (msg_for_send != NULL) {
            printf("\033[1FSending message: %s\n", msg_for_send);
            if (send_msg_to_all_clients(server, msg_for_send) < 0) {
                free(msg_for_send);
                RepeatedError("ServerMainloop");
                return -1;
            }
            free(msg_for_send);
            msg_for_send = NULL;
        }
        func_ret = server->on_flip(server);
        if (func_ret < 0) {
            RepeatedError("ServerMainloop");
            return -1;
        }
        if (func_ret == 0) return 0;
        __send_msg();
    }
    
}
#elif _WIN32
int ServerMainloop(Server *server) {
    IntoFunction("ServerMainloop");
    WSAEVENT evfd = WSACreateEvent();
    char* msg_for_send = NULL;
    if (evfd == WSA_INVALID_EVENT) {
        EventError("ServerMainloop", "Failed to create WSA event");
        return -1;
    }
    if (WSAEventSelect(server->listen_socket, evfd, FD_ACCEPT) == SOCKET_ERROR) {
        EventError("ServerMainloop", "Failed to select WSA event for listening socket");
        return -1;
    }
    int func_ret, nEvents = 1;
    WSAEVENT events[WSA_MAXIMUM_WAIT_EVENTS];
    Client *clients[WSA_MAXIMUM_WAIT_EVENTS];
    WSANETWORKEVENTS net_event;
    events[0] = evfd;
    while (1) {
        DWORD index = WSAWaitForMultipleEvents(nEvents, events, FALSE, 50, FALSE);
        if (RefreshErrorStream() < 0) {
            RepeatedError("ServerMainloop");
            return -1;
        }
        if (index == WSA_WAIT_FAILED) {
            EventError("ServerMainloop", "Failed to wait for events");
            return -1;
        }
        if (index != WSA_WAIT_TIMEOUT) {
            index -= WSA_WAIT_EVENT_0;
            if (index == 0) { // Events on listen socket
                SOCKET listen_sock = server->listen_socket;
                if (WSAEnumNetworkEvents(listen_sock, evfd, &net_event) == SOCKET_ERROR) {
                    EventError("ServerMainloop", "Failed to get network events");
                    return -1;
                }
                if (net_event.lNetworkEvents & FD_ACCEPT) { // listen socket have a new connection
                    if (net_event.iErrorCode[FD_ACCEPT_BIT] != 0) {
                        EventError("ServerMainloop", "Failed to accept the client");
                        return -1;
                    }
                    struct sockaddr_in addr;
                    socklen_t addr_len = sizeof(addr);
                    SOCKET client_socket = accept(listen_sock, (struct sockaddr*)&addr, &addr_len);
                    if (client_socket == INVALID_SOCKET) {
                        continue;
                    }
                    if (nEvents >= WSA_MAXIMUM_WAIT_EVENTS) {
                        if (closesocket(client_socket) == SOCKET_ERROR) {
                            SocketError("ServerMainloop", "Failed to close the client socket");
                            return -1;
                        }
                        continue;
                    }
                    Client *client = malloc(sizeof(Client));
                    if (client == NULL) {
                        MemoryError("ServerMainloop", "Failed to allocate memory for client");
                        return -1;
                    }
                    client->client_socket = client_socket;
                    client->addr = addr;
                    if (ClientInit(client, server->name, server->temp_socket, server->port, server->temp_roger_socket) == -1) {
                        closesocket(client_socket);
                        free(client);
                        RepeatedError("ServerMainloop");
                        return -1;
                    }
                    server->temp_socket = INVALID_SOCKET;
                    LinkAppend(&server->clients, client);
                    WSAEVENT client_event = WSACreateEvent();
                    if (client_event == WSA_INVALID_EVENT) {
                        LinkDelete(&server->clients, client);
                        closesocket(client_socket);
                        closesocket(client->target_socket);
                        EventError("ServerMainloop", "Failed to create WSA event for client");
                        return -1;
                    }
                    if (WSAEventSelect(client_socket, client_event, FD_READ | FD_CLOSE) == SOCKET_ERROR) {
                        LinkDelete(&server->clients, client);
                        closesocket(client_socket);
                        closesocket(client->target_socket);
                        EventError("ServerMainloop", "Failed to select WSA event for client");
                        return -1;
                    }
                    events[nEvents] = client_event;
                    clients[nEvents] = client;
                    nEvents++;
                    func_ret = server->on_connect(server, client);
                    if (func_ret < 0) {
                        RepeatedError("ServerMainloop");
                        return -1;
                    }
                    if (func_ret == 0) return 0;
                }
            } else {
                if (WSAEnumNetworkEvents(clients[index]->client_socket, events[index], &net_event) == SOCKET_ERROR) {
                    EventError("ServerMainloop", "Failed to get network events");
                    return -1;
                }
                if (net_event.lNetworkEvents & FD_READ) { // client send some message
                    if (net_event.iErrorCode[FD_READ_BIT] != 0) {
                        EventError("ServerMainloop", "Failed to read the client socket");
                        return -1;
                    }
                    char *buffer = malloc(CLIENT_BUFFER_SIZE);
                    if (buffer == NULL) {
                        MemoryError("ServerMainloop", "Failed to allocate memory for buffer");
                        return -1;
                    }
                    memset(buffer, 0, CLIENT_BUFFER_SIZE);
                    int buffer_len = CLIENT_BUFFER_SIZE;
                    int ret_len = recv(clients[index]->client_socket, buffer, CLIENT_BUFFER_SIZE, 0);
                    if (ret_len <= 0) {
                        if (WSACloseEvent(events[index]) == FALSE) {
                            EventError("ServerMainloop", "Failed to close the client event");
                            return -1;
                        }
                        delete_index_from_ptr_array(&events[0], index, nEvents);
                        if (closesocket(clients[index]->client_socket) == SOCKET_ERROR) {
                            SocketError("ServerMainloop", "Failed to close the client socket");
                            return -1;
                        }
                        func_ret = server->on_disconnect(server, clients[index], (ret_len < 0) ? 1 : 0);
                        closesocket(clients[index]->target_socket);
                        closesocket(clients[index]->client_socket);
                        LinkDelete(&server->clients, clients[index]);
                        ClientRelease(clients[index]);
                        delete_index_from_ptr_array((void *)&clients[0], index, nEvents);
                        nEvents--;
                        if (func_ret < 0) {
                            RepeatedError("ServerMainloop");
                            return -1;
                        }
                        if (func_ret == 0) return 0;
                    } else {
                        while (ret_len == CLIENT_BUFFER_SIZE && buffer[CLIENT_BUFFER_SIZE - 1] != '\0') {
                            buffer = (char*)realloc(buffer, buffer_len += CLIENT_BUFFER_SIZE);
                            if (buffer == NULL) {
                                MemoryError("ServerMainloop", "Failed to reallocate memory for buffer");
                                return -1;
                            }
                            memset(buffer + buffer_len - CLIENT_BUFFER_SIZE, 0, CLIENT_BUFFER_SIZE);
                            ret_len = recv(clients[index]->client_socket, buffer + buffer_len - CLIENT_BUFFER_SIZE, CLIENT_BUFFER_SIZE, 0);
                        }
                        if (ret_len > 0) {
                            if (clients[index]->message != NULL) free(clients[index]->message);
                            clients[index]->message = buffer;
                            func_ret = server->on_message(server, clients[index]);
                            if (func_ret < 0) {
                                RepeatedError("ServerMainloop");
                                return -1;
                            }
                            if (func_ret == 0) return 0;
                            continue;
                        }
                        func_ret = server->on_disconnect(server, clients[index], (ret_len < 0) ? 1 : 0);
                        closesocket(clients[index]->target_socket);
                        closesocket(clients[index]->client_socket);
                        LinkDelete(&server->clients, clients[index]);
                        ClientRelease(clients[index]);
                        delete_index_from_ptr_array((void *)&clients[0], index, nEvents);
                        if (func_ret < 0) {
                            RepeatedError("ServerMainloop");
                            return -1;
                        }
                        if (func_ret == 0) return 0;
                    }
                } else if (net_event.lNetworkEvents & FD_CLOSE) {
                    if (WSACloseEvent(events[index]) == FALSE) {
                        EventError("ServerMainloop", "Failed to close the client event");
                        return -1;
                    }
                    delete_index_from_ptr_array(&events[0], index, nEvents);
                    if (closesocket(clients[index]->client_socket) == SOCKET_ERROR) {
                        SocketError("ServerMainloop", "Failed to close the client socket");
                        return -1;
                    }
                    func_ret = server->on_disconnect(server, clients[index], 0);
                    closesocket(clients[index]->target_socket);
                    closesocket(clients[index]->client_socket);
                    LinkDelete(&server->clients, clients[index]);
                    ClientRelease(clients[index]);
                    delete_index_from_ptr_array((void *)&clients[0], index, nEvents);
                    nEvents--;
                    if (func_ret < 0) {
                        RepeatedError("ServerMainloop");
                        return -1;
                    }
                    if (func_ret == 0) return 0;
                }
            }
        }
        if ((msg_for_send = readLine(-1)) != NULL) {
            moveUpFirst();
            printf("Sending message: %s\n", msg_for_send);
            if (send_msg_to_all_clients(server, msg_for_send) < 0) {
                free(msg_for_send);
                RepeatedError("ServerMainloop");
                return -1;
            }
            free(msg_for_send);
            msg_for_send = NULL;
        }
        func_ret = server->on_flip(server);
        if (func_ret < 0) {
            RepeatedError("ServerMainloop");
            return -1;
        }
    }
}
#endif


int ClientInit(Client* client, const char* server_name, socket_t target_socket, uint16_t server_port, socket_t roger_socket) {
    IntoFunction("ClientInit");
    client->name = NULL;
    char* buffer = inet_ntoa(client->addr.sin_addr);
    client->ip = malloc(strlen(buffer) + 1);
    if (client->ip == NULL) {
        MemoryError("ClientInit", "Failed to allocate memory for ip");
        return -1;
    }
    strcpy(client->ip, buffer);
    client->message = NULL;
    // Initialize the target socket
    if SOCKET_IS_INVALID(target_socket) {
        char buf[64] = {0};
        int port;
        // Sleep(50);
        if (recv(client->client_socket, buf, 64, 0) < 0) {
            SocketError("ClientInit", "Failed to receive message from client");
            return -1;
        }
        if ((port = atoi(buf)) == 0 || port > 65535) {
            SocketError("ClientInit", "Invalid port number");
            return -1;
        }
        client->port = (uint16_t)port;
        Server temp_server = {.port = server_port, .name = (char*)server_name,
            #ifdef _WIN32
            .listen_socket = INVALID_SOCKET
            #elif __linux__
            .listen_socket = -1
            #endif
            };
        if (ConnectServerTo(&temp_server, client->ip, port, 0) == -1) {
            RepeatedError("ClientInit");
            return -1;
        }
        client->target_socket = temp_server.temp_socket;
        client->roger_socket = temp_server.temp_roger_socket;
    } else {
        client->target_socket = target_socket;
        client->roger_socket = roger_socket;
        client->port = target_port_temp;
    }
    return 0;
}

void ClientRelease(Client* client) {
    IntoFunction("ClientRelease");
    if (client->name != NULL) free(client->name);
    if (client->ip != NULL) free(client->ip);
    if (client->message != NULL) free(client->message);
    free(client);
}

int ConnectServerTo(Server *server, const char *ip, uint16_t port, int recursion) {
    IntoFunction("ConnectServerTo");
    printf("Connecting to %s:%d\n", ip, port);
    int socket_is_valid = !SOCKET_IS_INVALID(server->listen_socket); // If it's called by ClientInit, socket is invalid
    server->temp_roger_socket = socket(AF_INET, SOCK_STREAM, 0);
    server->temp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if SOCKET_IS_INVALID(server->temp_roger_socket) {
        SocketError("ConnectServerTo", "Failed to create target socket");
        return -1;
    }
    if SOCKET_IS_INVALID(server->temp_socket) {
        SocketError("ConnectServerTo", "Failed to create target socket");
        return -1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port + 1);
    addr.sin_addr.s_addr = inet_addr(ip);
    #ifdef __linux__
    if (connect(server->temp_roger_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) 
    #elif _WIN32
    if (connect(server->temp_roger_socket, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    #endif
    {
        #ifdef _WIN32
        closesocket(server->temp_roger_socket);
        #elif __linux__
        close(server->temp_roger_socket);
        #endif
        SocketError("ConnectServerTo", "Failed to connect to target socket");
        return -1;
    }
    int ret = send(server->temp_roger_socket, server->name, (int)strlen(server->name), 0);
    if (ret < 0) {
        #ifdef _WIN32
        closesocket(server->temp_roger_socket);
        #elif __linux__
        close(server->temp_roger_socket);
        #endif
        SocketError("ConnectServerTo", "Failed to send message to target socket");
        return -1;
    }
    addr.sin_port = htons(port);
    #ifdef __linux__
    if (connect(server->temp_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) 
    #elif _WIN32
    if (connect(server->temp_socket, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    #endif
    {
        #ifdef _WIN32
        closesocket(server->temp_socket);
        #elif __linux__
        close(server->temp_socket);
        #endif
        SocketError("ConnectServerTo", "Failed to connect to target socket");
        return -1;
    }
    if (socket_is_valid) {
        char buffer[64] = {0};
        sprintf(buffer, "%d", server->port);
        #ifdef __linux__
        if (send(server->temp_socket, buffer, (int)strlen(buffer), 0) < 0)
        #elif _WIN32
        if (send(server->temp_socket, buffer, (int)strlen(buffer), 0) == SOCKET_ERROR)
        #endif
        {
            #ifdef _WIN32
            closesocket(server->temp_socket);
            #elif __linux__
            close(server->temp_socket);
            #endif
            SocketError("ConnectServerTo", "Failed to send message to target socket");
            return -1;
        }
    }
    target_port_temp = port;
    server->temp_recrusion_connect = recursion;
    return 0;
}

int RecursionConnect(Server *server) {
    IntoFunction("RecursionConnect");
    LinkNode *cur = server->clients;
    // the last client is the new client
    // Step 1: Save all clients of the CALLER except the CALLEE
    int len = 0;
    while (cur != NULL) {
        len++;
        cur = cur->next;
    }
    Client **clients = malloc(sizeof(Client *) * len);
    len--;
    if (clients == NULL) {
        MemoryError("RecursionConnect", "Failed to allocate memory for clients");
        return -1;
    }
    cur = server->clients;
    for (int i = 0; i < len; i++) {
        clients[i] = (Client*)cur->data;
        cur = cur->next;
    }
    Client *target_client = (Client*)cur->data;
    // Step 2: request all clients of the CALLEE to connect to all clients of the CALLER
    char buffer[64];
    for (int i = 0; i < len; i++) {
        sprintf(buffer, "%s:%d", clients[i]->ip, clients[i]->port);
        printf("Requesting clients of (%s:%d) to connect to %s\n", target_client->ip, target_client->port, buffer);
        if (send_command(server, clients[i], "request_connect_clients_to", buffer) < 0) {
            free(clients);
            RepeatedError("RecursionConnect");
            return -1;
        }
    }
    // Step 3: request all clients of the CALLEE to connect to the CALLER
    sprintf(buffer, "%s:%d", server->ip, server->port);
    printf("Requesting clients of (%s:%d) to connect to %s\n", target_client->ip, target_client->port, buffer);
    if (send_command(server, target_client, "request_connect_clients_to", buffer) < 0) {
        free(clients);
        RepeatedError("RecursionConnect");
        return -1;
    }
    // Step 4: request the CALLEE to connect to the clients of the CALLER
    for (int i = 0; i < len; i++) {
        sprintf(buffer, "%s:%d", clients[i]->ip, clients[i]->port);
        printf("Requesting (%s:%d) to connect to %s\n", target_client->ip, target_client->port, buffer);
        if (send_command(server, target_client, "request_connect_to", buffer) < 0) {
            free(clients);
            RepeatedError("RecursionConnect");
            return -1;
        }
    }
}