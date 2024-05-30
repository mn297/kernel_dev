#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>

#define SOCKET_PATH "/tmp/example.sock"
#define MAX_CLIENTS 2
#define BUFFER_SIZE 256

int main()
{
    struct sockaddr_un addr;
    int server_fd, client_fd;
    int client_fds[MAX_CLIENTS] = {-1, -1}; // Array to hold client fds
    int ret;
    fd_set read_fds;
    int max_fd;
    char buffer[BUFFER_SIZE];

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

    while (1)
    {
        // Initialize the file descriptor set
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        max_fd = server_fd;

        // Add client sockets to the set
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (client_fds[i] > 0)
            {
                FD_SET(client_fds[i], &read_fds);
            }
            if (client_fds[i] > max_fd)
            {
                max_fd = client_fds[i];
            }
        }

        // Wait for activity on one of the sockets
        ret = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (ret < 0)
        {
            perror("select");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        // Check if it was for the server socket (new connection)
        if (FD_ISSET(server_fd, &read_fds))
        {
            client_fd = accept(server_fd, NULL, NULL);
            if (client_fd < 0)
            {
                perror("accept");
                close(server_fd);
                exit(EXIT_FAILURE);
            }

            printf("Accepted a new connection\n");

            // Add new client to the array of client fds
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (client_fds[i] == -1)
                {
                    client_fds[i] = client_fd;
                    break;
                }
            }
        }

        // Check if it was for a client socket (data from client)
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (client_fds[i] > 0 && FD_ISSET(client_fds[i], &read_fds))
            {
                ret = read(client_fds[i], buffer, BUFFER_SIZE - 1);
                if (ret < 0)
                {
                    perror("read");
                    close(client_fds[i]);
                    client_fds[i] = -1;
                }
                else if (ret == 0)
                {
                    // Client disconnected
                    printf("Client disconnected\n");
                    close(client_fds[i]);
                    client_fds[i] = -1;
                }
                else
                {
                    buffer[ret] = '\0';
                    printf("Received message: %s\n", buffer);

                    // Send a response to the client
                    const char *msg = "Hello, client!";
                    ret = write(client_fds[i], msg, strlen(msg));
                    if (ret < 0)
                    {
                        perror("write");
                        close(client_fds[i]);
                        client_fds[i] = -1;
                    }
                }
            }
        }
    }

    // Close the server socket
    close(server_fd);

    // Unlink the socket file
    unlink(SOCKET_PATH);

    return 0;
}
