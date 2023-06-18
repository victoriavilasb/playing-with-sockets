
#define MAX_CLIENT_CONNECTIONS 15
#define MAX_FILENAME_SIZE 200
#define BUFFER_SIZE 2048

struct command_control {
    int IdMsg;
    int IdSender;
    int IdReceiver;
    char Message[BUFFER_SIZE];
};


void send_and_recv_message(int sock, struct command_control *req, struct command_control * res);

void send_message(int sock, struct command_control *req);

void cast_users_message_to_array(const char* str, int array[]);

void cast_array_to_users_message(const int array[], char* str);

void
fatal_error(const char *msg);

int server_sockaddr_init(
    const char* ss_family,
    const char* portstr,
    struct sockaddr_storage *storage
);
