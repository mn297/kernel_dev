#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define FILE_PATH "/tmp/comm_file.txt"
#define BUFFER_SIZE 256

int main() {
    int sv[2]; // sv[0] and sv[1] are the file descriptors for the sockets
    char buffer[BUFFER_SIZE];

    // Create the socket pair
    if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sv) == -1) {
        perror("socketpair");
        exit(EXIT_FAILURE);
    }

    // Create or open the file for communication
    int file_fd = open(FILE_PATH, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (file_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process
        close(sv[0]); // Close the parent's end of the socket

        fd_set read_fds;
        while (1) {
            FD_ZERO(&read_fds);
            FD_SET(sv[1], &read_fds);
            FD_SET(file_fd, &read_fds);

            int max_fd = sv[1] > file_fd ? sv[1] : file_fd;
            int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

            if (activity == -1) {
                perror("select");
                exit(EXIT_FAILURE);
            }

            if (FD_ISSET(sv[1], &read_fds)) {
                int bytes = read(sv[1], buffer, sizeof(buffer) - 1);
                if (bytes > 0) {
                    buffer[bytes] = '\0';
                    printf("Child received from socket: %s\n", buffer);
                }
            }

            if (FD_ISSET(file_fd, &read_fds)) {
                lseek(file_fd, 0, SEEK_SET); // Reset file pointer
                int bytes = read(file_fd, buffer, sizeof(buffer) - 1);
                if (bytes > 0) {
                    buffer[bytes] = '\0';
                    printf("Child received from file: %s\n", buffer);
                }
            }
        }

        close(sv[1]); // Close the child's end of the socket
        close(file_fd); // Close the file descriptor
    } else {
        // Parent process
        close(sv[1]); // Close the child's end of the socket

        const char *msg1 = "Message from parent via socket";
        const char *msg2 = "Message from parent via file";

        if (write(sv[0], msg1, strlen(msg1)) == -1) {
            perror("write to socket");
            exit(EXIT_FAILURE);
        }

        // Write to the file
        if (write(file_fd, msg2, strlen(msg2)) == -1) {
            perror("write to file");
            exit(EXIT_FAILURE);
        }

        // Give the child process some time to read the messages
        sleep(1);

        close(sv[0]); // Close the parent's end of the socket
        close(file_fd); // Close the file descriptor
    }

    return 0;
}
