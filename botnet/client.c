// Simple client architecture
// Connects to host, reads message, disconnects

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

int main(void)
{
    int sock_fd, bytes_recv;
    char message[1024];
    struct addrinfo hints;
    struct addrinfo *server_info, *results;
    struct sockaddr_in *address;
    char buff[INET_ADDRSTRLEN];

    // Ensure struct is empty and load values
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo("127.0.0.1", "8080", &hints, &server_info);

    // Loop through results and connect with socket
    for (results = server_info; results != NULL; results = results->ai_next)
    {
        sock_fd = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
        connect(sock_fd, results->ai_addr, results->ai_addrlen);
        break;
    }

    // Retrieve IP address of server and print message
    // Currently returning incorrect value, I need to review this, but it's not
    // Needed for client, anyhow
    address = (struct sockaddr_in *)&results;
    struct in_addr ipAddr = address->sin_addr;
    inet_ntop(results->ai_family, &ipAddr, buff, sizeof(buff));
    printf("[+] Client connected to %s\n", buff);

    // Free struct as no longer in use
    freeaddrinfo(server_info);
    
    // Receive message, store bytes received
    bytes_recv = recv(sock_fd, message, 1024, 0);

    // Add NULL terminator to end of data and print data
    message[bytes_recv] = '\0';
    printf("[+] Client received: %s\n", message);

    // Close file descriptor
    close(sock_fd);

    // Exit
    return 0;
}
