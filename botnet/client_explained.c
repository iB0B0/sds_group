/* Simple client architecture
Connects to host, reads message, disconnects
Please note, all of the code below is absent of any error checking. This is for
2 reasons. Firstly, I just haven't implemented any, yet, but more importantly,
it keeps the code clean to help you understand. Another worthy note is the
absence of functions. Again, this is to better demonstrate the flow of the
program */

/* Not a lot to say here - they're the required header files. More header files
will be required as we add error checking and build additional functionality. */
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

    /* Same applies here as with the server, but I'll paste it here in case
    you're reading the client first. Chicken or the egg, right?
    Although it doesn't look like it, hints is a pointer. It's included in
    <netdb.h>. hints points to a struct of addrinfo. addrinfo contains the
    following information:
    struct addrinfo {
               int              ai_flags;
               int              ai_family;
               int              ai_socktype;
               int              ai_protocol;
               socklen_t        ai_addrlen;
               struct sockaddr *ai_addr;
               char            *ai_canonname;
               struct addrinfo *ai_next;
           };
    ai_family - the IP version, be that IPv4(INET) or IPv6(INET6)
    ai_socktype - stores the socket type, stream (SOCK_STREAM) or datagram (SOCK_DGRAM)
    ai_protocol - stores the protocol type
    ai_addrlen - stores the size of ai_addr in bytes
    ai_addr - stores a pointer to a sockaddr struct
    ai_next - points to next element in linked list

    You don't need to worry too much about manually populating each of these
    fields. A call to getaddrinfo() will do most of the work for you, as you
    will see below.

    If my explanations are short, you don't quite understand the structs, or you
    just want some more information, beej has some interesting stuff, or RTFM
    (read the man pages) */

    /* First, make a call to memset to '0' the structure. We don't want any
    residual data in our struct */
    memset(&hints, 0, sizeof(hints));

    /* Populate hints with desirable 'settings' before making a call to
    getaddrinfo() to populate the rest. I have set the IP version to IPv4 initally.
    We can make this IP version-agnostic, by setting to AF_UNSPEC. This allows
    for either IPv4 or IPv6. I have also set the socktype to be a SOCK_STREAM */
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    /* Given node and service, which identify an Internet host and a
    service, getaddrinfo() returns one or more addrinfo structures, each of which
    contains an Internet address that can be specified in a call to bind(2) or
    connect(2). Here, I have specified localhost (for testing purposes), and port
    8080. The information is stored in server_info struct. getaddrinfo() returns
    -1 on error. Error checking should be implemented here. */
    getaddrinfo("127.0.0.1", "8080", &hints, &server_info);

    /* Here we are looping through our structure to populate required arguments in
    socket() and connect(). To make a call to socket(), we need the IP version, socket
    type and protocol. As you can see below, we pull these from results struct.
    socket() returns a file descriptor (integer value). The  connect()  system call
    connects the socket referred to by the file descriptor sock_fd to the address
    specified by ai_addr. Both socket() and connect() return -1 on error and
    error checking should be implemented here. */
    for (results = server_info; results != NULL; results = results->ai_next)
    {
        sock_fd = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
        connect(sock_fd, results->ai_addr, results->ai_addrlen);
        break;
    }

    /* An explanation of what this code does, and why it's necessary can be found
    in server_explained.c. Essentially, it stores the IP of server for printing.
    This isn't necessary at all for the client, it was mostly for testing purposes
    (it's also proving to be buggy in client and I need to take  a deeper look at why).
    Anyhow, I'll leave it here for now. */
    address = (struct sockaddr_in *)&results;
    struct in_addr ipAddr = address->sin_addr;
    inet_ntop(results->ai_family, &ipAddr, buff, sizeof(buff));
    printf("[+] Client connected to %s\n", buff);

    /* After we have finished with a structure, it's considered good practise
    to free up the memory assigned. Beej says so! */
    freeaddrinfo(server_info);

    /* Call to recv() to read data from file descriptor refferred to by sock_fd,
    store the data in 'message' and read a maximum of 1024 bytes. '0' is the
    setting for flags. This can be left at 0 for all we do - I believe. recv()
    returns the number of bytes read. We store this in to a variable to give us
    the 'length' of data. */
    bytes_recv = recv(sock_fd, message, 1024, 0);

    /* During testing, I was getting some funky characters printed at the end of my
    message. Therefore, we append a null terminator to the end of the data receieved.
    This ensure we can print only the data sent by the server. */
    message[bytes_recv] = '\0';
    printf("[+] Client received: %s\n", message);

    /* Close file descriptor */
    close(sock_fd);

    /* Exit */
    return 0;
}
