#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/home/user3/example.sock"

int main() {
    int client_fd;
    struct sockaddr_un server_addr;
    const char *message = "Hello from client!";

    // Create client socket
    client_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (client_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set server address
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    // Connect to server
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_un)) == -1) {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // Send message to server
    if (write(client_fd, message, strlen(message)) == -1) {
        perror("write");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // Close socket
    close(client_fd);
    return 0;
}
