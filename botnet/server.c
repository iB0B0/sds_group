// Listens for connection and sends message to connected client

#include "simple_networking.h"
#include <poll.h>
#include <pthread.h>

void handle_connection(struct pollfd *pfds[], int *fd_count, int listener_fd, connection client_con, int *fd_size);
void print_connection(connection client_con, int new_fd);
void append_pfds(struct pollfd *pfds[], int new_fd, int *fd_count, int *fd_size);


int main(void)
{
    connection client_con = bind_socket("127.0.0.1", 8080);
    char data[1024];
    int listener_fd;

    int fd_count = 0;
    int fd_size = 5;
    struct pollfd *pfds = malloc(sizeof(*pfds) * fd_size);

    listener_fd = client_con.sock_fd;
    pfds[0].fd = listener_fd;
    pfds[0].events = POLLIN;

    fd_count++;

    printf("[+] Server awaiting connection...\n");

    // Main loop for accepting queued connections
    handle_connection(&pfds, &fd_count, listener_fd, client_con, &fd_size);


    if (fd_count == 4){
        if (!fork()) {

            while(1) {
                printf("[+] Enter command: ");
                fgets(data, sizeof(data), stdin);
                data[strcspn(data, "\n")] = 0;
                if (strcmp(data, "exit") == 0) {
                    exit(0);
                }
                printf("[CHILD] fd_count: %d\n", fd_count);
                for(int j = 1; j < fd_count; j++) {
                    printf("client: %d is ready on fd %d\n", j, pfds[j].fd);
                    send(pfds[j].fd, data, sizeof(data), 0);
                }
            }
        }
    }

    // Exit
    return 0;
}



void handle_connection(struct pollfd *pfds[], int *fd_count, int listener_fd, connection client_con, int *fd_size)
{
  socklen_t client_addr_size;
  int new_fd;

  while(1)
  {
      int poll_count = poll((*pfds), (*fd_count), -1);

      if (poll_count == -1) {
          perror("poll");
          exit(1);
      }

      // run through existing connections looking for data to read
      for(int i = 0; i < (*fd_count); i++) {

          //check if someone is ready to read
          if ((*pfds)[i].revents & POLLIN) { // one is ready to read

              if ((*pfds)[i].fd == listener_fd) {
                  // if listener is ready to read, handle new connection

                  // Accept queued connection and assign new file descriptor
                  client_addr_size = sizeof(client_con.client_addr);
                  new_fd = accept(client_con.sock_fd, (struct sockaddr *)&client_con.client_addr, &client_addr_size);

                  if (new_fd == -1) {
                      perror("accept");
                  }
                  else {
                    append_pfds(pfds, new_fd, fd_count, fd_size);
                    print_connection(client_con, new_fd);
                  }
              }
          }
      }
}
}

void print_connection(connection client_con, int new_fd)
{

  char buff[INET_ADDRSTRLEN];

  // Retrieve IP address of accepted client and print
  client_con.sa = (struct sockaddr_in *)&client_con.client_addr;
  struct in_addr ipAddr = client_con.sa->sin_addr;
  inet_ntop(client_con.client_addr.ss_family, &ipAddr, buff, sizeof(buff));
  printf("[+] Server: got connection from: %s on socket %d\n", buff, new_fd);
}

void append_pfds(struct pollfd *pfds[], int new_fd, int *fd_count, int *fd_size)
{
  if (*fd_size == *fd_count) {
    *fd_size *= 2;

    *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
  }

  (*pfds)[*fd_count].fd = new_fd;
  (*pfds)[*fd_count].events = POLLIN;

  (*fd_count)++;
}
