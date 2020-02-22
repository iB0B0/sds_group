/* Simple server architecture - detailed explanation
Listens for connection and sends message to connected client
Please note, all of the code below is absent of any error checking. This is for
2 reasons. Firstly, I just haven't implemented any, yet, but more importantly,
it keeps the code clean to help you understand. Another worthy note is the
absence of functions. Again, this is to better demonstrate the flow of the
program */

/* Not a lot to say here - they're the required header files. More header files
will be required as we add error checking and build additional functionality. */
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
    char buff[INET_ADDRSTRLEN];

    /* Although it doesn't look like it, hints is a pointer. It's included in
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
    socket() and bind(). To make a call to socket(), we need the IP version, socket
    type and protocol. As you can see below, we pull these from results struct.
    socket() returns a file descriptor (integer value). After creating a socket,
    we need to bind() the socket, reffered to by sock_fd, to an address, refferred
    to by ai_addr. socket() and bind() both return -1 on error and error checking
    should be implemented here too. */
    for (results = server_info; results != NULL; results = results->ai_next)
    {
        sock_fd = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
        bind(sock_fd, results->ai_addr, results->ai_addrlen);
        break;
    }

    /* After we have finished with a structure, it's considered good practise
    to free up the memory assigned. Beej says so! */
    freeaddrinfo(server_info);

    /* Not a lot to say here, it's pretty self explanatory. listens for a client
    on socket reffered to by sock_fd. Guess what? listen() returns -1 on error and
    error checking should be implemented here, too. */
    listen(sock_fd, 5);

    printf("[+] Server awaiting connection...\n");

    // Main loop for accepting queued connections
    while(1==1)
    {
        /* Here, we are making a call to accept(). Accept takes the first connection
        request from the queue of pending connections from the listening socket.
        accept() returns a new socket, independant of the existing socket (which
        continues to listen for new connections), we assign this to a different
        variable as so it can be kept seperate. */
        client_addr_size = sizeof(client_addr);
        connected_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &client_addr_size);

        /* This piece of code collects the connected client's IP address. It looks
        a little excessive, but it's just extracting the IP from the structure.
        The call to inet_ntop is necessary because of Byte ordering - you know...
        Big endian, Little endian, that madness. Anyway, ntop stands for
        'Network to presentation', there's a reverse function called pton. Basically,
        you’ll want to convert the numbers to Network Byte Order before they go
        out on the wire, and convert them to Host Byte Order as they come in off the wire.
        Have a quick google of this, it's useful to know - not only for this, but
        for computer architecture in general. */
        sa = (struct sockaddr_in *)&client_addr;
        struct in_addr ipAddr = sa->sin_addr;
        inet_ntop(client_addr.ss_family, &ipAddr, buff, sizeof(buff));
        printf("[+] Server: got connection from: %s\n", buff);

        /* Here, we make a call to fork(). This spawns a child process to handle
        the connected client, while the parent process continues to listen out
        for more connections */
        if (!fork()) {

            /* Close the listening file descriptor, child process does not need
            to listen on this socket */
            close(sock_fd);

            /* Write to file descriptor returned by accept(). '9' states the amount
            of bytes to write, while "conected" is the message to send. '0' is for
            setting flags, this can be set to 0 for all that we do - I believe. As
            with most things, send returns -1 on error and error checking should be
            implemented here. Also worth noting, is that send returns the number
            of bytes sent. Why is this important? It can send less than expected!
            Usually, there are no issues if it sends <1k, and it's up to us to ensure
            any missed data is resent. Knowing this could prevent a future headache. */
            send(connected_fd, "Connected", 9, 0);

            /* Close file descriptor associated with connected client */
            close(connected_fd);

            /* Exit child process */
            exit(0);
        }
        /* Close file descriptor on parent process as it's not needed by parent */
        close(connected_fd);
    }

    // Exit
    return 0;
}

/* I hope this helps you to understand how the server is implemented, albeit in
its most basic form. From here, we can begin to add functionality to the server.
If you don't understand any of the code don't hesitate to ask - I'll happily
point you in the right direction, or do my best to explain it personally. */
