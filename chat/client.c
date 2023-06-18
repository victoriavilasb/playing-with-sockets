#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <pthread.h>
#include "./common.h"

int addr_parser(const char *addr, const char *portstr, struct sockaddr_storage *storage);
void recv_message_handler(struct command_control res);
struct command_control message_mount(char *message, int my_id);
void * message_sender(void *arg);
void * message_receiver(void *arg);

void
usage(char **argv)
{
    printf("usage: %s <server IP> <server port>", argv[0]);
    printf("example: %s 127.0.0.1 3003", argv[0]);
    exit(EXIT_FAILURE);
}

struct thread_data {
    int sock;
    int client_id;
    int clients[MAX_CLIENT_CONNECTIONS];
};


int
main(int argc, char **argv)
{
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
    
    pthread_t send_thread, receive_thread;
    struct thread_data data;

    data.sock = sock;
    data.client_id = -1;
    for (int i = 0; i < MAX_CLIENT_CONNECTIONS; i++) {
        data.clients[i] = -1;
    }

    int send_thread_result = pthread_create(&send_thread, NULL, message_sender, &data);
    int receive_thread_result = pthread_create(&receive_thread, NULL, message_receiver, &data);
    
    if (send_thread_result != 0 || receive_thread_result != 0) {
        printf("Falha ao criar as threads.\n");
        exit(EXIT_FAILURE);
    }

    pthread_join(send_thread, NULL);
    pthread_join(receive_thread, NULL);

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

struct command_control
message_mount(char *message, int my_id)
{
    struct command_control req;
    if(strstr(message, "close connection") != NULL) {
        req.IdMsg = 2;
        req.IdSender = my_id;
    }

    return req; 
}

void *
message_sender(void *arg)
{
    for (;;) {
        struct thread_data *data = (struct thread_data *)arg;

        char cmd[BUFFER_SIZE];
        memset(cmd, 0, BUFFER_SIZE);

        if (fgets(cmd, sizeof(cmd), stdin) == NULL) {
            continue;// Sair do loop se não houver mais entrada
        }

        struct command_control req = message_mount(cmd, data->client_id);

        send_message(data->sock, &req);
    }

    pthread_exit(NULL);
}

void *
message_receiver(void *arg)
{
    for (;;) {
        struct thread_data *data = (struct thread_data *)arg;
        struct command_control res;
        ssize_t count = recv(data->sock, &res, sizeof(res), 0);
        if (count < 0) {
            printf("recv() failed");
            continue;
        }

        if (count == 0) {
            continue;
        }

        switch (res.IdMsg) {
            case 7:
                printf("%s\n", res.Message);
                exit(EXIT_FAILURE);
                continue;
            case 8:
                printf("%s\n", res.Message);
                exit(EXIT_SUCCESS);
                continue;
            case 4:
                cast_users_message_to_array(res.Message, data->clients);
                continue;
            case 6:
                // aqui precisa fazer a lógica para tirar o usuário da cash do cliente
                // e também adicionar quando for adicionado
                if (data->client_id == -1) {
                    
                    data->client_id = res.IdSender;
                }

                data->clients[res.IdSender-1] = res.IdSender;
                
                printf("%s\n", res.Message);

                continue;

        }

        memset(&res, 0, sizeof(res));

    }

    pthread_exit(NULL);
    
}