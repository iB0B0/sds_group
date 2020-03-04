// Simple server architecture
// Listens for connection and sends message to connected client

#include "simple_networking.h"


int main(void)
{
    connection client_con = bind_socket("127.0.0.1", 8080);
    socklen_t client_addr_size;
    char buff[INET_ADDRSTRLEN], data[1024];
    int connected_fd;

    // Listen for connection - allow 5 connection to queue
    listen(client_con.sock_fd, 5);
    
    printf("[+] Server awaiting connection...\n");

    // Main loop for accepting queued connections
    while(1==1)
    {
        // Accept queued connection and assign new file descriptor
        client_addr_size = sizeof(client_con.client_addr);
        connected_fd = accept(client_con.sock_fd, (struct sockaddr *)&client_con.client_addr, &client_addr_size);

        // Retrieve IP address of accepted client and print
        client_con.sa = (struct sockaddr_in *)&client_con.client_addr;
        struct in_addr ipAddr = client_con.sa->sin_addr;
        inet_ntop(client_con.client_addr.ss_family, &ipAddr, buff, sizeof(buff));
        printf("[+] Server: got connection from: %s\n", buff);

        // Create child process to handle connection w/ client
        if (!fork()) {

            // Close listening file descriptor, child process does not need
            close(client_con.sock_fd);

            while (1)
            {
              // Get user input and strip trailing newline '\n'
              printf("[+] Enter command: ");
              fgets(data, sizeof(data), stdin);
              data[strcspn(data, "\n")] = 0;

              // Send message to client, close file descriptor and exit child process
              send(connected_fd, data, sizeof(data), 0);
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
