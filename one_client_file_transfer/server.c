#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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

    struct sockaddr_storage cstorage;
    struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
    socklen_t caddrlen = sizeof(cstorage);
    ssize_t count;
    struct file_header header;
    char *buf = malloc(sizeof(header));

    FILE *fp;

    memset(&header, 0, sizeof(header));

    int csock = accept(sock, caddr, &caddrlen);
    if (csock < 0) {
        fatal_error("accept() failed"); 
    }

    while (1) {
        count = recv(csock, buf, sizeof(header), 0);
        if (count < 0) {
            fatal_error("recv() failed");
        } 

        deserialize_message(buf, &header);

        if (header.status == EXIT) {
            printf("connection closed\n");
            send(csock, buf, sizeof(header), 0);
            close(csock);
            exit(EXIT_SUCCESS);
        }

        if (header.status == STARTING) {
            printf("file %s received\n", header.filename);

            if(file_exists(header.filename)) {
                printf("file %s overwritten\n", header.filename);
            }
            
            fp = fopen(header.filename, "w"); 
        }

        if (header.status == IN_PROGRESS) {
            fp = fopen(header.filename, "a");
        }

        if (header.status == STARTING || header.status == IN_PROGRESS) {
            fprintf(fp, "%s", header.content);
            fclose(fp);
        }

        char * buffer = mount_message_to_send(header.filename, header.content, IN_PROGRESS); 
        send(csock, buffer, strlen(buffer)+1, 0);
    }

    close(csock);

    return 0;
}
