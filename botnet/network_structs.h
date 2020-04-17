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
typedef struct connection
{
    char *dest_ip;
    int dest_port;
    int sock_fd;
    int thread_id;
    struct pollfd *pfds;
    struct sockaddr_storage client_addr;
    struct sockaddr_in *sa;
    struct sockaddr *sa_accept;
    struct connection* next;
}connection;

typedef struct
{
    int protocol;
    char data[1024];
    int bytes_recv;
    struct machine dest_machine;
    struct machine source_machine;
} message;

// Below is a function which let you add a connection to the list
// Position 0: Add to end of list, Position 1: Add to "top" of list
// Where the position is greater than # of connections, it is placed at the end
// It expects the connection passed to it to be the head.
connection* create_connection(connection* current, int position)
{
    connection* new_con = (connection*)malloc(sizeof(connection));

    switch (position)
    {
        case 0:
            // Move through the list and find the last node
            while (current->next != NULL)
            {
                current = current->next;
            }

            // Set the current last node to point to our new connection
            current->next = new_con;
            new_con->next = NULL;
            break;

        case 1:
            // Set our new connection to point to the current head of the list
            new_con->next = current;
            break;
            
        default:
            // Subtract 2 to get the number of "hops" to our position
            if (position < 2)
            {
                return NULL;
            }
            position -= 2;

            // Hop to the right connection
            for (int i = 0; i < position; i++)
            {
                if (current->next == NULL)
                {
                    // Oops, we're at the end of the list. Stick it at the end.
                    current->next = new_con;
                    new_con->next = NULL;
                    break;
                }
                current = current->next;
            }

            // Track the next one along
            connection* next_con = current->next;

            // Insert it into the list
            current->next = new_con;
            new_con->next = next_con;
            
            break;
    }
    return new_con;
}

int count_connections(connection* head)
{
    int number = 1;
    while (head->next != NULL)
    {
        number++;
        head = head->next;
    }
    return number;
}