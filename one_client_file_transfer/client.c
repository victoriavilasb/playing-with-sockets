#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "./common.h"

#define MAX_FILENAME_SIZE 200
#define BUFFER_SIZE 1024

int
is_file_extension_valid(const char* filename)
{
    if (strstr(filename, ".txt") != NULL) {
        return 1;
    } else if (strstr(filename, ".py") != NULL) {
        return 1;
    } else if (strstr(filename, ".cpp") != NULL) {
        return 1;
    } else if (strstr(filename, ".c") != NULL) {
        return 1;
    } else if (strstr(filename, ".tex") != NULL) {
        return 1;
    } else if (strstr(filename, ".java") != NULL) {
        return 1;
    } else {
        return 0;
    }
}

struct file_header
receive_response(int sock)
{
    struct file_header header;
    char buf[sizeof(header)];
    ssize_t count = 0;

    memset(&header, 0, sizeof(header));
    memset(&buf, 0, sizeof(header));
    
    while(1) {
        count = recv(sock, buf, sizeof(header), 0);
        if (count == 0) {
            return header;
        } else if (count < 0) {
            fatal_error("recv() failed\n");
        } else {
            deserialize_message(buf, &header);

            if (header.status == EXIT) {
                close(sock);
                exit(EXIT_SUCCESS);
            }

            return header;
        }
    }
}

void
send_file(int sock, const char* filename)
{
    FILE *fb = fopen(filename, "rb");
    if (fb == NULL) {
        fatal_error("Couldn't open file");
    }

    char content_buffer[BUFFER_SIZE];
    size_t bytes_read;
    struct file_header header;
    enum file_transmission_status status = STARTING;
    char * message_buffer;
    int count;

    memset(&content_buffer, 0, BUFFER_SIZE);
    memset(&header, 0, sizeof(header));

    while ((bytes_read = fread(&content_buffer, 1, sizeof(content_buffer), fb)) > 0) {
        message_buffer = mount_message_to_send(filename, content_buffer, status);

        count = send(sock, message_buffer, sizeof(struct file_header), 0);
        if(count < 0) {
            fatal_error("Couldn't send file content");
        }

        memset(&content_buffer, 0, BUFFER_SIZE);

        receive_response(sock);
    }

    fclose(fb);
}

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
    char filename[MAX_FILENAME_SIZE];
    memset(filename, '\0', sizeof(filename));
    int count;
    struct file_header header;

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
        char command[20], subcommand[20];

        scanf("%s", command);

        if (strstr(command, "exit") != NULL) {
            char message_buffer[sizeof(header)];

            enum file_transmission_status exit_status = EXIT;
    
            memset(&header, 0, sizeof(header));
            memset(&message_buffer, 0, sizeof(header));

            memcpy(&header.status, &exit_status, sizeof(enum file_transmission_status));
            
            serialize_message(&header, message_buffer, sizeof(message_buffer)+1);

            count = send(sock, message_buffer, sizeof(header), 0);
            if(count < 0) {
                fatal_error("Couldn't exit message");
            }

            receive_response(sock);
        } else if (strstr(command, "select") != NULL) {
            scanf("%s", subcommand);
            scanf("%s", filename);

            if (!is_file_extension_valid(filename)) {
                printf("%s not valid!\n", filename);
                continue;
            }

            if (!file_exists(filename)) {
                printf("%s does not exist!\n", filename);
                memset(filename, '\0', sizeof(filename));
                continue;
            }

            printf("%s selected\n", filename);
        } else if (strstr(command, "send") != NULL) {
            scanf("%s", subcommand);

             if (strlen(filename) == 0) {
                printf("no file selected!\n");
                continue;
            }

            send_file(sock, filename);
        } else {
            return -1;
        }
    }
    
    return 0;
}

