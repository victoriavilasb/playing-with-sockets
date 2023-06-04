
#define MAX_FILENAME_SIZE 200
#define BUFFER_SIZE 1024

enum file_transmission_status {
    STARTING,
    IN_PROGRESS,
    EXIT
};

struct file_header {
    char filename[MAX_FILENAME_SIZE];
    char content[BUFFER_SIZE];
    enum file_transmission_status status;
};

int
file_exists(const char *filename);

void
fatal_error(const char *msg);

int server_sockaddr_init(
    const char* ss_family,
    const char* portstr,
    struct sockaddr_storage *storage
);

void
deserialize_message(char* source, struct file_header *destination);

void
serialize_message(struct file_header *source, char* destination, size_t dest_size);    

char *
mount_message_to_send(const char* filename, char *content, enum file_transmission_status status);