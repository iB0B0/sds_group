// Listens for connection and sends message to connected client
// to compile: gcc server.c -o server -lpthread

/*
Code is void of input sanitation and missing a lot of error checks.
Code does not update array when client disconnects.
Code has no functionality to exit cleanly.
*/

#include "simple_networking.h"
#include <poll.h>
#include <pthread.h>

// Function declarations
void* handle_connection(void* my_connection);
void* bot_command(void* current);
void print_connection(connection client_con, int new_fd);
int append_pfds(struct pollfd *pfds, int new_fd, int fd_count);

// Global Graceful Exit Flag
int exitflag = 0;

// Structure of arguments for use with bot_command and handle_connection functions
struct arguments
{
    int fd_count;
    int listener_fd;
    connection client_con;
    int *fd_size;
    struct pollfd *pfds;
};


static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(void)
{
    // Set the client connection to be the head of the list
    connection client_con = bind_socket("127.0.0.1", 8080);
    client_con.next = NULL;

    pthread_t control_thread, connection_thread;

    // Create thread to control botnet
    pthread_create(&control_thread, NULL, bot_command, &client_con);
    pthread_create(&connection_thread, NULL, handle_connection, &client_con);

    // Wait for threads to finish
    pthread_join(control_thread, NULL);
    pthread_join(connection_thread, NULL);

    // Exit
    return 0;
}

// Handles incomming connections and appends information to array
void* handle_connection(void* arg)
{
    connection* head = (connection *)arg;
    socklen_t client_addr_size;
    int new_fd;
    int fd_count = 1;

    printf("[+] Looking for connections\n");

    while(1)
    {
        connection* current = head;

        while (current != NULL)
        {
            // Poll the array for events and set .revents
            int rc = poll(head->pfds, fd_count, -1);

            // Check return code of poll(). Return error if -1
            if (rc == -1)
            {
                perror("[-] Error poll()");
                exit(1);
            }

            // Check if socket is ready to read
            if (head->pfds->revents & POLLIN)
            {
                // If listener is ready to read, handle new connection
                if (head->pfds->fd == head->sock_fd)
                {
                    // Accept queued connection and assign new file descriptor
                    client_addr_size = sizeof(head->client_addr);
                    new_fd = accept(head->sock_fd,
                        (struct sockaddr *)&head->client_addr, &client_addr_size);

                    // Check return code of accept(). Return error if -1
                    if (new_fd == -1)
                    {
                        perror("[-] Error accept()");
                    }
                    else
                    {
                        // Set lock
                        pthread_mutex_lock(&mutex);

                        // Add new connection information to list
                        connection* new_con = create_connection(head, 0);
                        new_con->pfds->fd = new_fd;
                        new_con->pfds->events = POLLIN;

                        // Add connection information to master array
                        fd_count = append_pfds(head->pfds, new_fd, fd_count);

                        // Release lock
                        pthread_mutex_unlock(&mutex);

                        // Print connection
                        printf("[+] New connection from:\n");
                    }
                }
            }
            current = current->next;
        }
    }
}

// Append new connection file descriptor to array, returns the number of file descriptors
int append_pfds(struct pollfd *pfds, int new_fd, int fd_count)
{
    // Increment count of assigned file descriptors
    fd_count++;

    // Increase size of array to accomodate new file descriptor
    pfds = realloc(pfds, sizeof(*pfds)*fd_count);

    // Add file descriptor to array and set event to check for read
    pfds[fd_count].fd = new_fd;
    pfds[fd_count].events = POLLIN;

    return fd_count;
}

// User control
// Will - this is where you will implement user interface
void* bot_command(void* arg)
{
    char data[1024];
    connection* head = (connection *)arg;
    connection* tmp = (connection *)malloc(sizeof(connection));
    while(1)
    {
        // Prompt user for input and remove trailing newline
        printf("[+] [show] connections, send to [single] client, send to [all] client or [exit]\n");
        printf(">> ");
        fgets(data, sizeof(data), stdin);
        data[strcspn(data, "\n")] = 0;

        // Display current list of connections
        if (strcmp(data, "show") == 0)
        {
            printf("[+] Connections:\n");

            // Set lock
            pthread_mutex_lock(&mutex);

            // Omit server info from description
            tmp = head;
            tmp = tmp->next;
            int i = 1;

            // Iterate through array and print client information
            while (tmp != NULL)
            {
                printf("client %d is on fd %d\n", i, tmp->sock_fd);
                tmp = tmp->next;
                i++;
            }
            if (i == 1)
            {
                printf("[-] No connections!\n");
            }
            
            // Release lock
            pthread_mutex_unlock(&mutex);
        }

        // Send command to ALL connections except listener
        else if (strcmp(data, "all") == 0)
        {

            // Prompt user for input and remove trailing newline
            printf("[+] Enter command: ");
            fgets(data, sizeof(data), stdin);
            data[strcspn(data, "\n")] = 0;

            // Set lock
            pthread_mutex_lock(&mutex);

            // Iterate through connections, omitting listener, and send data to each file descriptor
            tmp = head;
            tmp = tmp->next;
            while (tmp != NULL)
            {
                send(tmp->pfds->fd, data, sizeof(data), 0);
                tmp = tmp->next;
            }

            // Release lock
            pthread_mutex_unlock(&mutex);
        }

        // Send command to SINGLE client
        else if (strcmp(data, "single") == 0)
        {

            // Prompt user for input, remove trailing newline and convert input to int
            printf("[+] Enter client: ");
            fgets(data, sizeof(data), stdin);
            data[strcspn(data, "\n")] = 0;
            int num = atoi(data);

            // Prompt user for input and remove trailing newline
            printf("[+] Enter command: ");
            fgets(data, sizeof(data), stdin);
            data[strcspn(data, "\n")] = 0;

            // Set lock
            pthread_mutex_lock(&mutex);

            // Send data to client specified by user input, based on array index
            int i = 0;
            tmp = head;
            while (i < num && tmp != NULL)
            {
                tmp = tmp->next;
            }
            send(tmp->sock_fd, data, sizeof(data), 0);

            // Release lock
            pthread_mutex_unlock(&mutex);
        }

        // Exit control of bot
        else if (strcmp(data, "exit") == 0)
        {
            printf("Exiting...\n");
            exit(0);
        }
    }
}
