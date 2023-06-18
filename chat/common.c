#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"

void
fatal_error(const char *msg)
{
    printf("%s\n", msg);
    exit(EXIT_FAILURE);
}

int server_sockaddr_init(
    const char* ss_family,
    const char* portstr,
    struct sockaddr_storage *storage
)
{
    if (ss_family == NULL || portstr == NULL) {
        return -1;
    }

    uint16_t port = (u_int16_t)atoi(portstr);
    if (port <= 0) {
        return -1;
    }

    port = htons(port);

    memset(storage, 0, sizeof(*storage));
    if (strstr(ss_family, "v4") != NULL) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        memset(&addr4->sin_addr, 0, sizeof(addr4->sin_addr));
        addr4->sin_addr.s_addr = INADDR_ANY;
        return 0;
    } else if (strstr(ss_family, "v6") != NULL) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        addr6->sin6_addr = in6addr_any;
        return 0;
    } else {
        return -1;
    }
}


void
send_and_recv_message(int sock, struct command_control *req, struct command_control * res)
{
    
    int count = send(sock, req, sizeof(struct command_control), 0);
    if (count < 0) {
        fatal_error("could not send message");
    }

    // empty receive message
    memset(res, 0, sizeof(struct command_control));

    unsigned total = 0;
    while(1) {
        count = recv(sock, res, sizeof(struct command_control), 0);
        if (count < 0) {
            fatal_error("recv() failed");
        } 
        if (count == 0) {
            break;
        } 

        total += count;
        break;
    }
}


void
send_message(int sock, struct command_control *req)
{
    int count = send(sock, req, sizeof(struct command_control), 0);
    if (count < 0) {
        fatal_error("could not send message");
    }
}


void cast_users_message_to_array(const char* str, int array[]) {
    const int max_size = 15;
    int i = 0;
    
    for (int j=0; j < 15; j++) {
        array[j] = -1;
    }

    char* str_copy = strdup(str);
    char* token = strtok(str_copy, ",");
    while (token != NULL && i < max_size) {
        array[atoi(token)-1] = atoi(token)-1;
        i++;
        token = strtok(NULL, ",");
    }

    free(str_copy);
}

void cast_array_to_users_message(const int array[], char* str) {
    int offset = 0;
    for (int i = 0; i < MAX_CLIENT_CONNECTIONS; i++) {
        if (array[i] != -1) {
            offset += sprintf(str + offset, "%02d", i+1);

            if (i < MAX_CLIENT_CONNECTIONS - 1 && array[i + 1] != -1) {
                offset += sprintf(str + offset, ",");
            }
        }
    }
}
