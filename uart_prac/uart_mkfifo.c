// uart_emulator.c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#define UART_PATH "/tmp/tty"
#define BUFFER_SIZE 1024

void init_uart(const char *path)
{
    if (mkfifo(path, 0666) == -1)
    {
        if (errno != EEXIST)
        {
            perror("Failed to create FIFO");
            exit(EXIT_FAILURE);
        }
    }
}

void uart_write(const char *path, const char *message)
{
    int fd = open(path, O_WRONLY);
    if (fd == -1)
    {
        perror("Failed to open FIFO for writing");
        exit(EXIT_FAILURE);
    }

    ssize_t num_written = write(fd, message, strlen(message));
    if (num_written == -1)
    {
        perror("Failed to write to FIFO");
        close(fd);
        exit(EXIT_FAILURE);
    }
    printf("Wrote %zd bytes to %s\n", num_written, path);
    close(fd);
}

void uart_read(const char *path)
{
    char buffer[BUFFER_SIZE];
    int fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        perror("Failed to open FIFO for reading");
        exit(EXIT_FAILURE);
    }

    ssize_t num_read = read(fd, buffer, BUFFER_SIZE - 1);
    if (num_read == -1)
    {
        perror("Failed to read from FIFO");
        close(fd);
        exit(EXIT_FAILURE);
    }
    buffer[num_read] = '\0';
    printf("Read %zd bytes from %s: %s\n", num_read, path, buffer);
    close(fd);
}

int main()
{
    init_uart(UART_PATH);

    const char *message = "Hello, UART Emulator!\n";
    uart_write(UART_PATH, message);
    uart_read(UART_PATH);

    return EXIT_SUCCESS;
}
