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
#include <pthread.h>

#define SOCKET_PATH "/tmp/example.sock"
#define MAX_EVENTS 10
#define BUFFER_SIZE 256

typedef void (*callback_t)(int fd, struct epoll_event *event);

void handle_accept(int server_fd, struct epoll_event *event);
void handle_read_from_client(int client_fd, struct epoll_event *event);
void handle_write_to_client(int client_fd, struct epoll_event *event);

void *event_loop(void *arg);

int server_fd, epoll_fd;
int client_fd = -1;

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
    int ret;

    if (client_fd != -1)
    {
        printf("Already accepted a connection, client_fd=%d\n", client_fd);
        return;
    }

    client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
    if (client_fd == -1)
    {
        perror("accept");
        return;
    }

    printf("Accepted a new connection, client_fd=%d\n", client_fd);

    // Set client socket to non-blocking
    set_nonblocking(client_fd);

    // Add the new client_fd to the epoll instance
    struct epoll_event ev;
    struct epoll_event_data *client_data = malloc(sizeof(struct epoll_event_data));
    client_data->fd = client_fd;
    client_data->callback = handle_read_from_client;

    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = client_data;
    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
    if (ret < 0)
    {
        perror("handle_accept() epoll_ctl() ret < 0");
        close(client_fd);
        free(client_data);
    }
}

void handle_read_from_client(int client_fd, struct epoll_event *event)
{
    char buffer[BUFFER_SIZE];
    int ret;

    // Check if client_fd is valid
    if (client_fd == -1)
    {
        return;
    }

    ret = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (ret < 0)
    {
        if (errno != EAGAIN)
        {
            perror("read");
            close(client_fd);
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
            free(event->data.ptr);
        }
    }
    else if (ret == 0)
    {
        // Client disconnected
        close(client_fd);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        free(event->data.ptr);
        printf("Client disconnected,client_fd=%d, setting to -1\n", client_fd);
        client_fd = -1;
    }
    else
    {
        buffer[ret] = '\0';
        printf("Received from client: %s\n", buffer);

        // What is this part for???
        // Prepare to write response
        // struct epoll_event ev;
        // struct epoll_event_data *client_data = malloc(sizeof(struct epoll_event_data));
        // client_data->fd = client_fd;
        // client_data->callback = handle_write_to_client;

        // ev.events = EPOLLOUT | EPOLLET;
        // ev.data.ptr = client_data;
        // ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
        // if (ret == -1)
        // {
        //     perror("epoll_ctl");
        //     close(client_fd);
        //     free(client_data);
        // }
    }
}

// Not used at the moment. Need to implement socket pair.
void handle_write_to_client(int client_fd, struct epoll_event *event)
{
    const char *msg = "Hello, client! from handle_write_to_client()";
    int ret;

    if (client_fd == -1)
    {
        return;
    }

    ret = write(client_fd, msg, strlen(msg));
    if (ret < 0)
    {
        printf("write() to client_fd%d failed\n", client_fd);
        perror("write");
        close(client_fd);
        epoll_ctl(event->data.fd, EPOLL_CTL_DEL, client_fd, NULL);
        free(event->data.ptr);
    }
    else
    {
        // USELESS???
        // Switch back to reading
        // struct epoll_event ev;
        // struct epoll_event_data *client_data = malloc(sizeof(struct epoll_event_data));
        // client_data->fd = client_fd;
        // client_data->callback = handle_read_from_client;

        // ev.events = EPOLLIN | EPOLLET;
        // ev.data.ptr = client_data;
        // if (epoll_ctl(event->data.fd, EPOLL_CTL_MOD, client_fd, &ev) == -1)
        // {
        //     perror("epoll_ctl");
        //     close(client_fd);
        //     free(client_data);
        // }
    }
}

int main()
{
    struct sockaddr_un addr;
    int ret;
    struct epoll_event ev;

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
    epoll_fd = epoll_create1(0 | EPOLL_CLOEXEC);
    if (epoll_fd == -1)
    {
        perror("epoll_create1");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Add server_fd to the epoll instance
    struct epoll_event_data *server_data = malloc(sizeof(struct epoll_event_data));
    server_data->fd = server_fd;
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

    // Epoll handler thread.
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, event_loop, &epoll_fd) != 0)
    {
        perror("pthread_create");
        close(server_fd);
        close(epoll_fd);
        return EXIT_FAILURE;
    }

    // Send data to client
    while (1)
    {
        sleep(0.5);
        const char *msg = "Hello, client! from main()";
        int ret;
        if (client_fd == -1)
        {
            continue;
        }
        ret = write(client_fd, msg, strlen(msg));
        if (ret < 0)
        {
            perror("write");
            close(client_fd);
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
            free(server_data);
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

void *event_loop(void *arg)
{
    int epoll_fd = *(int *)arg;
    struct epoll_event events[MAX_EVENTS];

    while (1)
    {
        int n_fds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n_fds == -1)
        {
            perror("epoll_wait");
            close(epoll_fd);
            pthread_exit(NULL);
        }

        for (int i = 0; i < n_fds; i++)
        {
            struct epoll_event_data *event_data = (struct epoll_event_data *)events[i].data.ptr;
            event_data->callback(event_data->fd, &events[i]);
        }
    }
}
