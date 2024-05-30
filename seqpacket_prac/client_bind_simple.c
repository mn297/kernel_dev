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
    int client_fd;
    char buffer[256];
    int ret;

    // Create a UNIX domain socket
    client_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (client_fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Zero out the address structure
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Connect to the server
    ret = connect(client_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
    if (ret < 0)
    {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // Receive message from server
    ret = read(client_fd, buffer, sizeof(buffer) - 1);
    if (ret < 0)
    {
        perror("read");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // Null-terminate the received data to safely print it as a string
    buffer[ret] = '\0';
    printf("Received message: %s\n", buffer);

    // Close the client socket
    close(client_fd);

    return 0;
}
