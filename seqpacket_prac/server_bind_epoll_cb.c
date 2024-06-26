#define _GNU_SOURCE

// Standard library headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

// System headers for sockets and Unix domain sockets
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

// System headers for epoll
#include <sys/epoll.h>

// Headers for signal handling and backtrace
#include <signal.h>
#include <execinfo.h>

// Headers for threading
#include <pthread.h>

// Custom header for helpers
#include "helpers.h"

typedef void (*callback_t)(int fd, struct epoll_event *event);

void handle_accept(int server_fd, struct epoll_event *event);
void handle_read_from_client(int client_fd, struct epoll_event *event);
void handle_write_to_client(int client_fd, struct epoll_event *event);

void *event_loop(void *arg);

int server_fd, epoll_fd;
int client_fd = -1;
pthread_mutex_t client_fd_mutex = PTHREAD_MUTEX_INITIALIZER;

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
    int new_client_fd;
    int ret;

    new_client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
    if (new_client_fd == -1)
    {
        perror("accept");
        return;
    }

    printf("Accepted a new connection, new_client_fd=%d\n", new_client_fd);

    // Set client socket to non-blocking
    set_nonblocking(new_client_fd);

    // Add the new client_fd to the epoll instance
    struct epoll_event ev;
    struct epoll_event_data *client_data = malloc(sizeof(struct epoll_event_data));
    client_data->fd = new_client_fd;
    client_data->callback = handle_read_from_client;

    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = client_data;
    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_client_fd, &ev);
    if (ret == -1)
    {
        perror("handle_accept() epoll_ctl() failed");
        close(new_client_fd);
        free(client_data);
    }
    else
    {
        // Properly close and cleanup the previous client connection if exists
        pthread_mutex_lock(&client_fd_mutex);
        if (client_fd != -1)
        {
            close(client_fd);
            printf("Closed previous client connection, client_fd=%d\n", client_fd);
        }
        client_fd = new_client_fd; // Update the global client_fd
        pthread_mutex_unlock(&client_fd_mutex);
    }
}

void handle_read_from_client(int fd, struct epoll_event *event)
{
    char buffer[BUFFER_SIZE];
    int ret;
    int server_fd_inode = get_inode(server_fd);
    int client_fd_inode = get_inode(fd);

    printf("handle_read_from_client()\n");
    printf("    server_fd=%d, inode=%ld\n", server_fd, server_fd_inode);
    printf("    client_fd=%d, inode=%ld\n", fd, client_fd_inode);
    if (fd == -1)
    {
        printf("    Client disconnected\n");
        return;
    }

    ret = read(fd, buffer, BUFFER_SIZE - 1);
    if (ret < 0)
    {
        int err = errno;
        const char *description;
        const char *name = errno_to_name(err, &description);
        printf("    read() client_fd=%d failed, errno=%s=%d=%s=%s\n",
               fd, name, err, description, strerror(errno));
        if (err == EAGAIN || err == EWOULDBLOCK)
        {
            printf("errno=%s\n", strerror(errno));
            return;
        }
        else
        {
            printf("errno=%s\n", strerror(errno));
            perror("read");
            close(fd);
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
            free(event->data.ptr);
            pthread_mutex_lock(&client_fd_mutex);
            if (fd == client_fd)
                client_fd = -1;
            pthread_mutex_unlock(&client_fd_mutex);
        }
    }
    else if (ret == 0)
    {
        // Client disconnected
        printf("    Client disconnected\n");
        close(fd);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
        free(event->data.ptr);
        pthread_mutex_lock(&client_fd_mutex);
        if (fd == client_fd)
            client_fd = -1;
        pthread_mutex_unlock(&client_fd_mutex);
    }
    else
    {
        buffer[ret] = '\0';
        printf("    received=%s\n", buffer);
    }
}

// Not used at the moment. Need to implement socket pair.
// void handle_write_to_client(int client_fd, struct epoll_event *event)
// {
//     const char *msg = "Hello, client! from handle_write_to_client()";
//     int ret;

//     if (client_fd == -1)
//     {
//         return;
//     }

//     ret = write(client_fd, msg, strlen(msg));
//     if (ret < 0)
//     {
//         printf("write() to client_fd%d failed\n", client_fd);
//         perror("write");
//         close(client_fd);
//         epoll_ctl(event->data.fd, EPOLL_CTL_DEL, client_fd, NULL);
//         free(event->data.ptr);
//     }
//     else
//     {
//         return;
//     }
// }

// Function to handle SIGSEGV
void handle_sigsegv(int sig)
{
    void *array[10];
    size_t size;

    printf("Caught signal %d (%s)\n", sig, strsignal(sig));

    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
    // You can place a breakpoint here to debug
    exit(EXIT_FAILURE);
}

int main()
{
    struct sockaddr_un addr;
    int ret;
    struct epoll_event ev;

    // Register the SIGSEGV handler
    if (signal(SIGSEGV, handle_sigsegv) == SIG_ERR)
    {
        perror("signal");
        return EXIT_FAILURE;
    }

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
    const char *msg = "Hello, client! from main()";
    while (1)
    {
        int ret;
        // 1.1 second delay
        usleep(1100000);

        pthread_mutex_lock(&client_fd_mutex);
        int fd_to_write = client_fd;
        pthread_mutex_unlock(&client_fd_mutex);

        if (fd_to_write == -1)
        {
            printf("No client connected\n");
            continue;
        }

        ret = write(fd_to_write, msg, strlen(msg));

        // Industry standard if-else-if block for SOCK_SEQPACKET, checking everything including different errno flags combinations.
        if (ret < 0)
        {
            int err = errno;
            const char *description;
            const char *name = errno_to_name(err, &description);
            printf("write() to client_fd=%d failed, errno=%s=%d=%s=%s\n",
                   fd_to_write, name, err, description, strerror(errno));
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                printf("errno=%s\n", strerror(errno));
                continue;
            }
            else
            {
                printf("errno=%s\n", strerror(errno));
                perror("write");
                close(fd_to_write);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd_to_write, NULL);
                pthread_mutex_lock(&client_fd_mutex);
                if (fd_to_write == client_fd)
                    client_fd = -1;
                pthread_mutex_unlock(&client_fd_mutex);
            }
        }
        else if (ret == 0)
        {
            int err = errno;
            const char *description;
            const char *name = errno_to_name(err, &description);
            printf("write() to client_fd=%d failed, errno=%s=%d=%s=%s\n",
                   fd_to_write, name, err, description, strerror(errno));

            close(fd_to_write);
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd_to_write, NULL);
            pthread_mutex_lock(&client_fd_mutex);
            if (fd_to_write == client_fd)
                client_fd = -1;
            pthread_mutex_unlock(&client_fd_mutex);
        }
        else
        {
            printf("write() to client_fd=%d succeeded\n", fd_to_write);
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
