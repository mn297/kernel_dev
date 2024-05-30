#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <pthread.h>

#define SOCKET_PATH "/tmp/example.sock"
#define NUM_THREADS 3

void *client_thread_func(void *arg)
{
    struct sockaddr_un addr;
    int client_fd;
    char buffer[256];
    int ret;
    int thread_id = *((int *)arg);

    // Create a UNIX domain socket
    client_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (client_fd == -1)
    {
        perror("socket");
        pthread_exit(NULL);
    }

    // Zero out the address structure
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Connect to the server
    ret = connect(client_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
    if (ret < 0)
    {
        perror("connect");
        close(client_fd);
        pthread_exit(NULL);
    }

    // Send a message to the server
    char msg[256];
    snprintf(msg, sizeof(msg), "Hello from client %d", thread_id);
    ret = write(client_fd, msg, strlen(msg));
    if (ret < 0)
    {
        perror("write");
        close(client_fd);
        pthread_exit(NULL);
    }

    // Receive message from server
    ret = read(client_fd, buffer, sizeof(buffer) - 1);
    if (ret < 0)
    {
        perror("read");
        close(client_fd);
        pthread_exit(NULL);
    }

    buffer[ret] = '\0';
    printf("Client %d received message: %s\n", thread_id, buffer);

    // Close the client socket
    close(client_fd);

    pthread_exit(NULL);
}

int main()
{
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    int ret;

    // Create threads
    for (int i = 0; i < NUM_THREADS; i++)
    {
        thread_ids[i] = i + 1;
        ret = pthread_create(&threads[i], NULL, client_thread_func, &thread_ids[i]);
        if (ret != 0)
        {
            fprintf(stderr, "Error creating thread %d: %s\n", i + 1, strerror(ret));
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
