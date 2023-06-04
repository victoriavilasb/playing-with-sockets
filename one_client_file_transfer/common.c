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

int
file_exists(const char *filename)
{
    if (access(filename, R_OK) != -1) {
        return 1;
    }

    return 0;
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
serialize_message(struct file_header *source, char* destination, size_t dest_size)
{
    unsigned char buffer[sizeof(*source)];
    int offset = 0;

    memcpy(buffer + offset, source->filename, sizeof(source->filename));
    offset += sizeof(source->filename);

    memcpy(buffer + offset, source->content, sizeof(source->content));
    offset += sizeof(source->content);

    memcpy(buffer + offset, &source->status, sizeof(enum file_transmission_status));

    offset += sizeof(enum file_transmission_status);

    memcpy(destination, buffer, dest_size);

    return;
}

char *
mount_message_to_send(const char* filename, char *content, enum file_transmission_status status)
{
    struct file_header header;
    char *buffer = malloc(sizeof(header));

    memset(&header, 0, sizeof(header));

    memcpy(&header.status, &status, sizeof(enum file_transmission_status));
    memcpy(header.filename, filename, MAX_FILENAME_SIZE);
    memcpy(header.content, content, BUFFER_SIZE);
    
    serialize_message(&header, buffer, sizeof(header));

    return buffer;
}

void
deserialize_message(char* source, struct file_header *destination)
{
    int offset = 0;

    memcpy(destination->filename, source + offset, MAX_FILENAME_SIZE);
    offset += MAX_FILENAME_SIZE;

    memcpy(destination->content, source + offset, BUFFER_SIZE);
    offset += BUFFER_SIZE;

    memcpy(&destination->status, source + offset, sizeof(enum file_transmission_status));
    offset += sizeof(enum file_transmission_status);

    return;
}