#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "./common.h"

#define MAX_PENDING_CONNECTIONS 10
#define BUFFER_SIZE 1024

void
usage(char **argv)
{
    printf("usage: %s <v4 v6> <server port>", argv[0]);
    printf("example: %s v4 127.0.0.1", argv[0]);
    exit(EXIT_FAILURE);
}

void *
client_thread(void *csock) {
    ssize_t count;
    char message[BUFFER_SIZE];
    memset(&message, 0, sizeof(message));


    while (1) {
        count = recv(csock, message, BUFFER_SIZE - 1, 0);
        if (count < 0) {
            fatal_error("recv() failed");
        } 

        printf("message received from client: %s\n", message);

        send(csock, message, strlen(message)+1, 0);
    }
    
    close(csock);
    pthread_exit(EXIT_SUCCESS);

}

int
main(int argc, char **argv)
{
    int sock;

    if (argc < 3) {
        printf("Usage: %s <ip_version> <port>\n", argv[0]);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        fatal_error("failed to parse server address");
    }

    sock = socket(storage.ss_family, SOCK_STREAM, 0);
    if (sock < 0) {
        fatal_error("socket() failed");
    }

    struct sockaddr *servAddr = (struct sockaddr *)(&storage);
    
    if (bind(sock, servAddr, sizeof(storage)) != 0) {
        fatal_error("bind() failed");
    }

    if(listen(sock, MAX_PENDING_CONNECTIONS)) {
        fatal_error("listen() failed");
    }

    while(1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        char message[BUFFER_SIZE];
        memset(&message, 0, sizeof(message));

        int csock = accept(sock, caddr, &caddrlen);
        if (csock < 0) {
            fatal_error("accept() failed"); 
        }

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, csock);
    }


    return 0;
}


