CC = gcc
CFLAGS = -Wall -Wextra -pedantic -lm -pthread -D_POSIX_C_SOURCE=200809L -std=c11
COMMON_OBJS = common.o
SERVER_OBJS = server.o $(COMMON_OBJS)
CLIENT_OBJS = client.o $(COMMON_OBJS)

all: server client

server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -g -o server_environment/server $(SERVER_OBJS)

client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -g -o client $(CLIENT_OBJS)

common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c

server.o: server.c common.h
	$(CC) $(CFLAGS) -c server.c

client.o: client.c common.h
	$(CC) $(CFLAGS) -c client.c

clean:
	rm -f *.o server client server_environment/server
