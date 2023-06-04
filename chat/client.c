#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "./common.h"

#define BUFFER_SIZE 1024

void
usage(char **argv)
{
    printf("usage: %s <server IP> <server port>", argv[0]);
    printf("example: %s 127.0.0.1 3003", argv[0]);
    exit(EXIT_FAILURE);
}

int
addr_parser(const char *addr, const char *portstr, struct sockaddr_storage *storage)
{
    if (addr == NULL || portstr == NULL) {
        return -1;
    }

    uint16_t port = (u_int16_t)atoi(portstr);
    if (port <= 0) {

        return -1;
    }

    port = htons(port);
    struct in_addr inaddr4;
    struct in6_addr inaddr6;
    if (inet_pton(AF_INET, addr, &inaddr4)) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;  
        return 0;
    } else if (inet_pton(AF_INET6, addr, &inaddr6)) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_port = port;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_addr = inaddr6;

        return 0;
    } else {
        return -1;
    }
}

int
main(int argc, char **argv)
{
    int count;

    if (argc < 3) {
        usage(argv);
    }

    struct sockaddr_storage storage;

    if (addr_parser(argv[1], argv[2], &storage) != 0) {
        fatal_error("inet_pton() failed. invalid address");
    }

    int sock = socket(storage.ss_family, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        fatal_error("socket() failed");
    }
  
    struct sockaddr *servAddr = (struct sockaddr *)(&storage);
    if (connect(sock, servAddr, sizeof(storage)) < 0) {
        fatal_error("connect() failed");
    }

    while (1) {
        char message[BUFFER_SIZE];
        memset(message, 0, BUFFER_SIZE);

        if (fgets(message, sizeof(message), stdin) == NULL) {
            break;  // Sair do loop se não houver mais entrada
        }

        size_t message_len = strlen(message);

        if (message_len > 0 && message[message_len - 1] == '\n') {
            message[message_len - 1] = '\0';  // Remover o caractere de nova linha
        }

        count = send(sock, message, strlen(message)+1, 0);
        if (count <= 0) {
            fatal_error("could not send message");
        }

        memset(message, 0, BUFFER_SIZE);
        unsigned total =0;
        while(1) {
            count = recv(sock, message + total, BUFFER_SIZE - total, 0);
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
    
    return 0;
}

