// Simple client architecture
// Connects to host, reads message, disconnects

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>

int main(void)
{
    int sock_fd, bytes_recv;
    char message[1024];
    struct sockaddr_in address;

    // Manually load values in to address
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &(address.sin_addr));

    // Create socket
    sock_fd = socket(PF_INET, SOCK_STREAM, 0);

    // Connect to socket
    if (connect(sock_fd, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
      close(sock_fd);
      return 1;
    }

    printf("[+] Client connected.\n");

    // Main while loop to accept messages from server until exit received
    while (strcmp(message, "exit") != 0)
    {
      // Receive message, store bytes received
      bytes_recv = recv(sock_fd, message, 1024, 0);

      // Add NULL terminator to end of data and print data
      message[bytes_recv] = '\0';
      printf("[+] Client received: %s\n", message);
    }

    printf("[+] Exit message reveived. Closing connection.\n");

    // Close file descriptor
    close(sock_fd);

    // Exit
    return 0;
}
