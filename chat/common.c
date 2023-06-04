#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"

#define BUFFER_SIZE 1024

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