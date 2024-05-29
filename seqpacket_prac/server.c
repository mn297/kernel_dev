#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <errno.h>
#include <pthread.h>

#define SOCKET_PATH "./example.sock"
#define MAX_EVENTS 10
#define BUFFER_SIZE 256

static int epoll_fd;
static int server_fd;
static int fd_data_socket;
static pthread_t server_thread;

static void server_socket_cleanup(void);
static void server_socket_process_client_message(void);
static void server_socket_process_new_connection(void);
static void* server_thread_func(void* param);

void handle_client_message(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t num_bytes = read(client_fd, buffer, BUFFER_SIZE - 1);

    if (num_bytes > 0) {
        buffer[num_bytes] = '\0';
        printf("Received message: %s\n", buffer);
    } else if (num_bytes == 0) {
        // Client disconnected
        close(client_fd);
    } else {
        perror("read");
    }
}

void setup_socket_address(struct sockaddr_un *addr, const char *path) {
    memset(addr, 0, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strncpy(addr->sun_path, path, sizeof(addr->sun_path) - 1);
}

int create_and_bind_socket(const char *path) {
    int sockfd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr;
    setup_socket_address(&addr, path);
    unlink(path); // Ensure the socket does not already exist

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

void setup_epoll(int epoll_fd, int sockfd, void (*callback)(void)) {
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = callback;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &event) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }
}

void accept_new_connection(void) {
    fd_data_socket = accept(server_fd, NULL, NULL);
    if (fd_data_socket == -1) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // Set up epoll to monitor the new client connection
    setup_epoll(epoll_fd, fd_data_socket, server_socket_process_client_message);
}

void server_socket_process_client_message(void) {
    handle_client_message(fd_data_socket);
}

void server_socket_process_new_connection(void) {
    accept_new_connection();
}

void server_socket_cleanup(void) {
    if (fd_data_socket != 0) {
        close(fd_data_socket);
    }
    close(server_fd);
    unlink(SOCKET_PATH);
}

void* server_thread_func(void* param) {
    (void)param;
    struct epoll_event events[MAX_EVENTS];
    int event_count;

    printf("Server thread started\n");

    while (1) {
        event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (event_count == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < event_count; i++) {
            void (*callback)(void) = events[i].data.ptr;
            callback();
        }
    }

    return NULL;
}

pthread_t server_socket_init(void) {
    int ret;

    // Create and bind the server socket
    server_fd = create_and_bind_socket(SOCKET_PATH);

    // Listen for incoming connections
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Create epoll instance
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Add server socket to epoll
    setup_epoll(epoll_fd, server_fd, server_socket_process_new_connection);

    // Start server thread
    ret = pthread_create(&server_thread, NULL, server_thread_func, NULL);
    if (ret != 0) {
        fprintf(stderr, "pthread_create failed: %s\n", strerror(ret));
        close(server_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    ret = pthread_setname_np(server_thread, "server_thread");
    if (ret != 0) {
        fprintf(stderr, "pthread_setname_np failed: %s\n", strerror(ret));
        close(server_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server socket initialization done\n");

    return server_thread;
}

int main() {
    server_socket_init();

    // Join the server thread (wait for it to finish)
    pthread_join(server_thread, NULL);

    return 0;
}
