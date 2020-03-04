struct machine
{
    int machine_id;
    int is_master;
    int is_local_machine;
    char *ip_address;
    int open_port;
    int preferred_protocol;
};

// Mostly self explanatory, but for the server the dest_ values are actually locally bound values.
typedef struct
{
    char *dest_ip;
    int dest_port;
    int sock_fd;
    struct sockaddr_storage client_addr;
    struct sockaddr_in *sa;
    
}connection;

typedef struct
{
    int protocol;
    char data[1024];
    int bytes_recv;
    struct machine dest_machine;
    struct machine source_machine;
} message;