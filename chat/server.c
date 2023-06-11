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
#define MAX_CLIENT_CONNECTIONS 15

struct connections {
    int count;
    int clients[MAX_CLIENT_CONNECTIONS];
    pthread_mutex_t mutex;
};

struct thread_data {
    int csock;
    struct connections *conns;
};

void
usage(char **argv)
{
    printf("usage: %s <v4 v6> <server port>", argv[0]);
    printf("example: %s v4 127.0.0.1", argv[0]);
    exit(EXIT_FAILURE);
}

void * client_thread(void *csock);
void broadcast(struct connections *conns, struct command_control *req, int sender);
void init_conns(struct connections *conns);
int fill_available_conn(struct connections *conns, int csock);

int
main(int argc, char **argv)
{
    int sock;
    struct connections conns;
    conns.count = 0;
    if (pthread_mutex_init(&conns.mutex, NULL) != 0) {
        fatal_error("mutex initialization failed");
    }

    init_conns(&conns);

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
        struct thread_data td;
        socklen_t caddrlen = sizeof(cstorage);

        char message[BUFFER_SIZE];
        memset(&message, 0, sizeof(message));

        int csock = accept(sock, caddr, &caddrlen);
        if (csock < 0) {
            fatal_error("accept() failed"); 
        }

        pthread_t tid;

        td.csock = csock;
        td.conns = &conns;
        pthread_create(&tid, NULL, client_thread, &td);
    }


    return 0;
}

void *
client_thread(void *arg) 
{
    ssize_t count;
    struct thread_data *data = (struct thread_data *)arg;
    struct connections *conns = data->conns;
    struct command_control res, req;
    char msg[BUFFER_SIZE];
    char * m;
    int conn_number;

    while (1) {
        count = recv(data->csock, &res, sizeof(res), 0);
        if (count < 0) {
            fatal_error("recv() failed");
        } 

        memset(&req, 0, sizeof(req));
        switch (res.IdMsg) {
            case 1:
                if (conns->count == 15) {
                    req.IdMsg = 7;
                    m = "User limit exceeded";
                    memcpy(req.message, m, strlen(m)+1);

                    send_message(data->csock, &req);
                    close(data->csock);
                    pthread_exit(NULL);
                }

                // para incrementar o valor de count a gente precisa locar para garantir que não haja race condition
                pthread_mutex_lock(&conns->mutex);
                conns->count++;
                conn_number = fill_available_conn(conns, data->csock);
                conns->clients[conn_number] = data->csock;
                pthread_mutex_unlock(&conns->mutex);     


                printf("User %02d added\n", conn_number+1);
                
                sprintf(msg, "User %02d joined the group", conn_number+1);
                memcpy(req.message, msg, strlen(msg)+1);     

                broadcast(data->conns, &req, data->csock);
                break;
            default:
                printf("invalid message");
                pthread_exit(NULL);
        }

        memset(&res, 0, sizeof(res));
        memset(msg, 0, strlen(msg)+1);
    }
    
    close(data->csock);
    pthread_exit(EXIT_SUCCESS);
}

void
broadcast(struct connections *conns, struct command_control *req, int sender) {
    req->IdMsg = 6;
    for (int i = 0; i < MAX_CLIENT_CONNECTIONS; i++) {
        if (conns->clients[i] > 0) {
            send_message(conns->clients[i], req);
        }
    }
}

void
init_conns(struct connections *conns)
{
    for (int i = 0; i < MAX_CLIENT_CONNECTIONS; i++) {
        conns->clients[i] = -1;
    }
}

int
fill_available_conn(struct connections *conns, int csock)
{
    for (int i = 0; i < MAX_CLIENT_CONNECTIONS; i++) {
        if (conns->clients[i] == -1) {
            conns->clients[i] = csock;
            return i;
        }
    }

    return -1;
}
