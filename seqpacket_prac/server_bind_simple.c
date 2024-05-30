#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/example.sock"

int main()
{
    struct sockaddr_un addr;
    int server_fd, client_fd;
    int ret;

    // Create a UNIX domain socket
    server_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (server_fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Zero out the address structure
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Unlink the socket path if it already exists
    ret = unlink(SOCKET_PATH);
    if (ret < 0 && errno != ENOENT)
    {
        perror("unlink");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the specified address
    ret = bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
    if (ret < 0)
    {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    ret = listen(server_fd, 5);
    if (ret < 0)
    {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on %s\n", SOCKET_PATH);

    // Accept a connection
    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0)
    {
        perror("accept");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Accepted a connection\n");

    // Communicate with the client (for demonstration, just send a message)
    const char *msg = "Hello, client!";
    ret = write(client_fd, msg, strlen(msg));
    if (ret < 0)
    {
        perror("write");
        close(client_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Close the client and server sockets
    close(client_fd);
    close(server_fd);

    // Unlink the socket file
    unlink(SOCKET_PATH);

    return 0;
}
