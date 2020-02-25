// Simple server architecture
// Listens for connection and sends message to connected client

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

int main(void)
{
    int sock_fd, connected_fd;
    struct addrinfo hints;
    struct addrinfo *server_info, *results;
    struct sockaddr_storage client_addr;
    struct sockaddr_in *sa;
    socklen_t client_addr_size;
    char buff[INET_ADDRSTRLEN], message[1024];

    // Ensure struct is empty and load values
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo("127.0.0.1", "8080", &hints, &server_info);

    // Loop through results and bind to socket
    for (results = server_info; results != NULL; results = results->ai_next)
    {
        sock_fd = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
        bind(sock_fd, results->ai_addr, results->ai_addrlen);
        break;
    }

    // Free struct as no longer in use
    freeaddrinfo(server_info);

    // Listen for connection - allow 5 connection to queue
    listen(sock_fd, 5);

    printf("[+] Server awaiting connection...\n");

    // Main loop for accepting queued connections
    while(1==1)
    {
        // Accept queued connection and assign new file descriptor
        client_addr_size = sizeof(client_addr);
        connected_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &client_addr_size);

        // Retrieve IP address of accepted client and print
        sa = (struct sockaddr_in *)&client_addr;
        struct in_addr ipAddr = sa->sin_addr;
        inet_ntop(client_addr.ss_family, &ipAddr, buff, sizeof(buff));
        printf("[+] Server: got connection from: %s\n", buff);

        // Create child process to handle connection w/ client
        if (!fork()) {

            // Close listening file descriptor, child process does not need
            close(sock_fd);

            while (1)
            {
              // Get user input and strip trailing newline '\n'
              printf("[+] Enter command: ");
              fgets(message, sizeof(message), stdin);
              message[strcspn(message, "\n")] = 0;

              // Send message to client, close file descriptor and exit child process
              send(connected_fd, message, sizeof(message), 0);
            }
            close(connected_fd);
            exit(0);
        }
        // Close file descriptor on parent process
        close(connected_fd);
    }

    // Exit
    return 0;
}
