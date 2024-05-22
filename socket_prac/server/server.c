#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <time.h>

#define SOCKET_PATH "/tmp/server.socket"
#define BUFFER_SIZE 256

void log_message(const char *message) {
    time_t now = time(NULL);
    char *time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0'; // Remove newline
    printf("[%s] %s\n", time_str, message);
}

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    int bytes_read;

    while ((bytes_read = read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        log_message(buffer);

        // Respond to client
        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response), "Echo: %s", buffer);
        write(client_fd, response, strlen(response) + 1);
    }

    if (bytes_read == -1) {
        perror("read error");
    }

    close(client_fd);
    log_message("Client disconnected.");
}

void cleanup() {
    unlink(SOCKET_PATH);
}

void handle_signal(int sig) {
    if (sig == SIGINT) {
        cleanup();
        exit(0);
    }
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_un server_addr;

    // Setup signal handler for cleanup
    signal(SIGINT, handle_signal);

    // Create socket
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    // Set up the address structure
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    // Bind the socket to the address
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind error");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 5) == -1) {
        perror("listen error");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    log_message("Server is waiting for connections...");

    // Accept and handle connections
    while (1) {
        if ((client_fd = accept(server_fd, NULL, NULL)) == -1) {
            perror("accept error");
            continue;
        }

        log_message("Client connected.");

        if (fork() == 0) {
            close(server_fd);
            handle_client(client_fd);
            exit(0);
        } else {
            close(client_fd);
        }
    }

    cleanup();
    return 0;
}
