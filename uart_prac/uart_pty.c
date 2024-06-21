#define _XOPEN_SOURCE 600 // Ensure POSIX functions are declared

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pty.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>

#define BUFFER_SIZE 1024

int main()
{
    int master_fd, slave_fd;
    char slave_name[100];
    char read_buffer[BUFFER_SIZE];
    char write_buffer[BUFFER_SIZE];
    struct termios termios_p;
    ssize_t num_read;

    if (openpty(&master_fd, &slave_fd, slave_name, NULL, NULL) == -1)
    {
        perror("openpty");
        return EXIT_FAILURE;
    }

    tcgetattr(slave_fd, &termios_p);
    cfmakeraw(&termios_p);
    tcsetattr(slave_fd, TCSANOW, &termios_p);

    printf("Pseudo-terminal created: %s\n", slave_name);

    strcpy(write_buffer, "Hello, UART Emulator!\n");
    if (write(slave_fd, write_buffer, strlen(write_buffer)) == -1)
    {
        perror("write to slave");
        close(master_fd);
        close(slave_fd);
        return EXIT_FAILURE;
    }
    printf("Wrote to %s\n", slave_name);

    num_read = read(master_fd, read_buffer, BUFFER_SIZE - 1);
    if (num_read == -1)
    {
        perror("read from master");
        close(master_fd);
        close(slave_fd);
        return EXIT_FAILURE;
    }
    read_buffer[num_read] = '\0';
    printf("Read from %s: %s\n", slave_name, read_buffer);

    close(master_fd);
    close(slave_fd);
    return EXIT_SUCCESS;
}
