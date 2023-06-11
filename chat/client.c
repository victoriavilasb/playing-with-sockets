#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include "./common.h"

int
addr_parser(const char *addr, const char *portstr, struct sockaddr_storage *storage);

void
usage(char **argv)
{
    printf("usage: %s <server IP> <server port>", argv[0]);
    printf("example: %s 127.0.0.1 3003", argv[0]);
    exit(EXIT_FAILURE);
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

    // connection message
    struct command_control req;

    req.IdMsg = 1;
    send_message(sock, &req);
    
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sock, &readSet);
    FD_SET(STDIN_FILENO, &readSet);

    while (1) {
        fd_set tempSet = readSet;

        // Aguardar a atividade em um dos descritores de arquivo
        if (select(sock + 1, &tempSet, NULL, NULL, NULL) < 0) {
            perror("select() failed");
            break;
        }

        if (FD_ISSET(sock, &tempSet)) {
            struct command_control res;
            ssize_t count = recv(sock, &res, sizeof(res), 0);
            if (count <= 0) {
                printf("recv() failed");
                break;
            }

             if (res.IdMsg == 7) {
                printf("%s\n", res.message);
                exit(EXIT_FAILURE);
            } 

            printf("%s\n", res.message);
        }

        if (FD_ISSET(STDIN_FILENO, &tempSet)) {
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
    }
    
    return 0;
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
