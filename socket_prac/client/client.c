#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>

#define SOCKET_PATH "/tmp/server.socket"
#define BUFFER_SIZE 256

void log_message(const char *message) {
    time_t now = time(NULL);
    char *time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0'; // Remove newline
    printf("[%s] %s\n", time_str, message);
}

void communicate_with_server(const char *message) {
    int client_fd;
    struct sockaddr_un client_addr;

    // Create socket
    if ((client_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    // Set up the address structure
    memset(&client_addr, 0, sizeof(struct sockaddr_un));
    client_addr.sun_family = AF_UNIX;
    strncpy(client_addr.sun_path, SOCKET_PATH, sizeof(client_addr.sun_path) - 1);

    // Connect to the server
    if (connect(client_fd, (struct sockaddr*)&client_addr, sizeof(struct sockaddr_un)) == -1) {
        perror("connect error");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    log_message("Connected to server.");

    // Send message to the server
    write(client_fd, message, strlen(message) + 1);

    // Read response from the server
    char buffer[BUFFER_SIZE];
    int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        log_message(buffer);
    }

    close(client_fd);
    log_message("Disconnected from server.");
}

int main() {
    const char *message = "Hello from client!";
    communicate_with_server(message);
    return 0;
}
