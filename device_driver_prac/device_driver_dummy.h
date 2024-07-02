#ifndef DEVICE_DRIVER_DUMMY_H
#define DEVICE_DRIVER_DUMMY_H

#include <linux/poll.h> // for poll_table

// Define the frame size and queue size as used in the implementation
#define FRAME_SIZE 256
#define QUEUE_SIZE 10

// Declare the functions that will be implemented in device_driver_dummy.c
ssize_t dummy_driver_write(const char *buffer, size_t len);
ssize_t dummy_driver_read(char *buffer);
unsigned int dummy_driver_poll(struct file *file, poll_table *wait);

#endif // DEVICE_DRIVER_DUMMY_H
