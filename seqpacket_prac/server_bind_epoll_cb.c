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

typedef void (*callback_t)(int fd, struct epoll_event *event);

void handle_accept(int server_fd, struct epoll_event *event);
void handle_client_read(int client_fd, struct epoll_event *event);
void handle_client_write(int client_fd, struct epoll_event *event);

struct epoll_event_data
{
    int fd;
    callback_t callback;
};

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

void handle_accept(int server_fd, struct epoll_event *event)
{
    struct sockaddr_un addr;
    socklen_t addrlen = sizeof(addr);
    int client_fd;

    client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
    if (client_fd == -1)
    {
        perror("accept");
        return;
    }

    printf("Accepted a new connection\n");

    // Set client socket to non-blocking
    set_nonblocking(client_fd);

    // Add the new client_fd to the epoll instance
    struct epoll_event ev;
    struct epoll_event_data *client_data = malloc(sizeof(struct epoll_event_data));
    client_data->fd = client_fd;
    client_data->callback = handle_client_read;

    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = client_data;
    if (epoll_ctl(event->data.fd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
    {
        perror("epoll_ctl");
        close(client_fd);
        free(client_data);
    }
}

void handle_client_read(int client_fd, struct epoll_event *event)
{
    char buffer[BUFFER_SIZE];
    int ret;

    ret = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (ret < 0)
    {
        if (errno != EAGAIN)
        {
            perror("read");
            close(client_fd);
            epoll_ctl(event->data.fd, EPOLL_CTL_DEL, client_fd, NULL);
            free(event->data.ptr);
        }
    }
    else if (ret == 0)
    {
        // Client disconnected
        printf("Client disconnected\n");
        close(client_fd);
        epoll_ctl(event->data.fd, EPOLL_CTL_DEL, client_fd, NULL);
        free(event->data.ptr);
    }
    else
    {
        buffer[ret] = '\0';
        printf("Received message: %s\n", buffer);

        // Prepare to write response
        struct epoll_event ev;
        struct epoll_event_data *client_data = malloc(sizeof(struct epoll_event_data));
        client_data->fd = client_fd;
        client_data->callback = handle_client_write;

        ev.events = EPOLLOUT | EPOLLET;
        ev.data.ptr = client_data;
        if (epoll_ctl(event->data.fd, EPOLL_CTL_MOD, client_fd, &ev) == -1)
        {
            perror("epoll_ctl");
            close(client_fd);
            free(client_data);
        }
    }
}

void handle_client_write(int client_fd, struct epoll_event *event)
{
    const char *msg = "Hello, client!";
    int ret;

    ret = write(client_fd, msg, strlen(msg));
    if (ret < 0)
    {
        perror("write");
        close(client_fd);
        epoll_ctl(event->data.fd, EPOLL_CTL_DEL, client_fd, NULL);
        free(event->data.ptr);
    }
    else
    {
        // Switch back to reading
        struct epoll_event ev;
        struct epoll_event_data *client_data = malloc(sizeof(struct epoll_event_data));
        client_data->fd = client_fd;
        client_data->callback = handle_client_read;

        ev.events = EPOLLIN | EPOLLET;
        ev.data.ptr = client_data;
        if (epoll_ctl(event->data.fd, EPOLL_CTL_MOD, client_fd, &ev) == -1)
        {
            perror("epoll_ctl");
            close(client_fd);
            free(client_data);
        }
    }
}

int main()
{
    struct sockaddr_un addr;
    int server_fd, epoll_fd;
    int ret;
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
    struct epoll_event_data *server_data = malloc(sizeof(struct epoll_event_data));
    server_data->fd = epoll_fd;
    server_data->callback = handle_accept;

    ev.events = EPOLLIN;
    ev.data.ptr = server_data;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1)
    {
        perror("epoll_ctl");
        close(server_fd);
        close(epoll_fd);
        free(server_data);
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
            struct epoll_event_data *event_data = (struct epoll_event_data *)events[i].data.ptr;
            event_data->callback(event_data->fd, &events[i]);
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
