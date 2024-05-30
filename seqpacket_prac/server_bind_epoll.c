#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define SOCKET_PATH "/tmp/example.sock"
#define MAX_EVENTS 10
#define BUFFER_SIZE 256

void set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
}

int main()
{
    struct sockaddr_un addr;
    int server_fd, client_fd, epoll_fd;
    int ret;
    char buffer[BUFFER_SIZE];
    struct epoll_event ev, events[MAX_EVENTS];

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

    // Set server socket to non-blocking
    set_nonblocking(server_fd);

    // Create an epoll instance
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        perror("epoll_create1");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Add server_fd to the epoll instance
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1)
    {
        perror("epoll_ctl");
        close(server_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        int n_fds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n_fds == -1)
        {
            perror("epoll_wait");
            close(server_fd);
            close(epoll_fd);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < n_fds; i++)
        {
            if (events[i].data.fd == server_fd)
            {
                // Accept a new connection
                client_fd = accept(server_fd, NULL, NULL);
                if (client_fd == -1)
                {
                    perror("accept");
                    continue;
                }

                printf("Accepted a new connection\n");

                // Set client socket to non-blocking
                set_nonblocking(client_fd);

                // Add the new client_fd to the epoll instance
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
                {
                    perror("epoll_ctl");
                    close(client_fd);
                    continue;
                }
            }
            else
            {
                // Handle data from a client
                client_fd = events[i].data.fd;
                ret = read(client_fd, buffer, BUFFER_SIZE - 1);
                if (ret < 0)
                {
                    if (errno != EAGAIN)
                    {
                        perror("read");
                        close(client_fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    }
                }
                else if (ret == 0)
                {
                    // Client disconnected
                    printf("Client disconnected\n");
                    close(client_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                }
                else
                {
                    buffer[ret] = '\0';
                    printf("Received message: %s\n", buffer);

                    // Send a response to the client
                    const char *msg = "Hello, client!";
                    ret = write(client_fd, msg, strlen(msg));
                    if (ret < 0)
                    {
                        perror("write");
                        close(client_fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    }
                }
            }
        }
    }

    // Close the server socket
    close(server_fd);

    // Close the epoll file descriptor
    close(epoll_fd);

    // Unlink the socket file
    unlink(SOCKET_PATH);

    return 0;
}
