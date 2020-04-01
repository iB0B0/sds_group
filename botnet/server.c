// Listens for connection and sends message to connected client
// Messy, needs tidying up.
// Code currently not working.
// Issue with blocking, will fix (cody)

#include "simple_networking.h"
#include <poll.h>
#include <pthread.h>

void* handle_connection(void *arguments);
void print_connection(connection client_con, int new_fd);
void append_pfds(struct pollfd *pfds[], int new_fd, int *fd_count, int *fd_size);
void* bot_command(void *args_main);

struct arguments
{
  int fd_count;
  int listener_fd;
  connection client_con;
  int *fd_size;
  struct pollfd *pfds;
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(void)
{
    connection client_con = bind_socket("127.0.0.1", 8080);
    int listener_fd;
    pthread_t thread1, thread2;

    int fd_count = 0;
    int fd_size = 5;
    struct pollfd *pfds = malloc(sizeof(*pfds) * fd_size);

    listener_fd = client_con.sock_fd;
    pfds[0].fd = listener_fd;
    pfds[0].events = POLLIN;

    fd_count++;

    printf("[+] Server awaiting connection...\n");

    struct arguments *args;
    args = malloc(sizeof(struct arguments));

    args->pfds = pfds;
    args->fd_count = fd_count;
    args->listener_fd = listener_fd;
    args->client_con = client_con;
    args->fd_size = &fd_size;


    // Main loop for accepting queued connections
    pthread_create(&thread1, NULL, handle_connection, (void *)args);

    // User control here
    pthread_create(&thread2, NULL, bot_command, (void *)args);

    pthread_join(thread1, NULL);

    // Exit
    return 0;
}

void* handle_connection(void *args_main)
{
  printf("ENTERED THREAD\n");

  socklen_t client_addr_size;
  int new_fd;
  struct arguments *args = (struct arguments*)args_main;

  while(1)
  {
      pthread_mutex_lock(&mutex);

      int poll_count = poll(args->pfds, args->fd_count, -1);

      pthread_mutex_unlock(&mutex);

      if (poll_count == -1) {
          perror("poll");
          exit(1);
      }

      // run through existing connections looking for data to read
      for(int i = 0; i < args->fd_count; i++) {

          //check if someone is ready to read
          if (args->pfds[i].revents & POLLIN) { // one is ready to read

              if (args->pfds[i].fd == args->listener_fd) {
                  // if listener is ready to read, handle new connection

                  // Accept queued connection and assign new file descriptor
                  client_addr_size = sizeof(args->client_con.client_addr);
                  new_fd = accept(args->client_con.sock_fd, (struct sockaddr *)&args->client_con.client_addr, &client_addr_size);

                  if (new_fd == -1) {
                      perror("accept");
                  }
                  else {
                    pthread_mutex_lock(&mutex);
                    append_pfds(&args->pfds, new_fd, &args->fd_count, args->fd_size);
                    pthread_mutex_unlock(&mutex);
                    print_connection(args->client_con, new_fd);
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


// this is incomplete, will finish prior to next meeting
void* bot_command(void *args_main)
{
  char data[1024];

  struct arguments *args = (struct arguments*)args_main;

  while(1)
  {
    printf("[+] Send to one client or to all?\n");
    printf(">> ");
    fgets(data, sizeof(data), stdin);
    data[strcspn(data, "\n")] = 0;
    if (strcmp(data, "all") == 0) {
      printf("[+] Enter command: ");
      fgets(data, sizeof(data), stdin);
      data[strcspn(data, "\n")] = 0;
      if (strcmp(data, "exit") == 0) {
          exit(0);
      }
      pthread_mutex_lock(&mutex);
      //for(int j = 1; j < args->fd_count; j++) {
          send(args->pfds[2].fd, data, sizeof(data), 0);
      //}
      pthread_mutex_unlock(&mutex);
    }
  }
}
