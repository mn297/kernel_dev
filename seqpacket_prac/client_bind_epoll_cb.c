#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>

#define SOCKET_PATH "/tmp/example.sock"
#define MAX_EVENTS 10
#define BUFFER_SIZE 256

typedef void (*callback_t)(int fd, struct epoll_event *event);

struct epoll_event_data
{
    int fd;
    callback_t callback;
};

int sock_fd, epoll_fd;
pthread_mutex_t sock_fd_mutex = PTHREAD_MUTEX_INITIALIZER;

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

void handle_from_server(int fd, struct epoll_event *event)
{
    char buffer[BUFFER_SIZE];
    int ret;

    ret = read(fd, buffer, BUFFER_SIZE - 1);
    if (ret > 0)
    {
        buffer[ret] = '\0';
        printf("Received from server: %s\n", buffer);
    }
    else if (ret == 0 || (ret < 0 && errno != EAGAIN))
    {
        // Server closed connection or read error
        printf("Server closed connection or error occurred\n");
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        free(event->data.ptr);
        pthread_exit(NULL); // Exit the thread on a critical error or server disconnect
    }
}

void add_to_epoll(int epoll_fd, int fd, callback_t callback)
{
    struct epoll_event ev;
    struct epoll_event_data *data = malloc(sizeof(struct epoll_event_data));
    int ret;

    data->fd = fd;
    data->callback = callback;

    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = data;
    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);

    if (ret == -1)
    {
        perror("epoll_ctl");
        close(fd);
        free(data);
        exit(EXIT_FAILURE);
    }
}

void *epoll_thread_func(void *arg)
{
    int epoll_fd = *(int *)arg;
    struct epoll_event events[MAX_EVENTS];
    int n_fds, i;

    while (1)
    {
        n_fds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n_fds == -1)
        {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (i = 0; i < n_fds; i++)
        {
            struct epoll_event_data *event_data = (struct epoll_event_data *)events[i].data.ptr;
            event_data->callback(event_data->fd, &events[i]);
        }
    }
}

void write_to_server(const char *msg)
{
    pthread_mutex_lock(&sock_fd_mutex);
    int ret = write(sock_fd, msg, strlen(msg));
    pthread_mutex_unlock(&sock_fd_mutex);

    if (ret < 0)
    {
        perror("write");
        // Handle write errors if necessary
    }
    else
    {
        printf("write_to_server() %s\n", msg);
    }
}

int main()
{
    struct sockaddr_un addr;
    pthread_t epoll_thread;
    int ret;

    // Create client socket
    sock_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (sock_fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set socket to non-blocking
    set_nonblocking(sock_fd);

    // Setup address structure
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Connect to server
    ret = connect(sock_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
    if (ret == -1)
    {
        perror("connect");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Create epoll instance
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        perror("epoll_create1");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Add the socket to the epoll instance using the callback model
    add_to_epoll(epoll_fd, sock_fd, handle_from_server);

    // Start epoll event handling in a separate thread
    if (pthread_create(&epoll_thread, NULL, epoll_thread_func, &epoll_fd) != 0)
    {
        perror("pthread_create");
        close(sock_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    // Example of sending messages to the server
    const char *msg = "Hello, server!";
    while (1)
    {
        sleep(2);
        write_to_server(msg);
    }
    pthread_join(epoll_thread, NULL); // Optionally wait for the thread to finish

    // Cleanup (unreachable code in normal execution)
    close(sock_fd);
    close(epoll_fd);
    return 0;
}
