// uart_chardev.c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define DEVICE_NAME "uart_emulator"
#define BUFFER_SIZE 1024

static int major_number;
static char device_buffer[BUFFER_SIZE];
static struct class *device_class;
static struct cdev uart_cdev;

static ssize_t device_read(struct file *file, char __user *user_buffer, size_t len, loff_t *offset)
{
    ssize_t bytes_read;

    if (*offset >= BUFFER_SIZE)
        return 0;
    if (*offset + len > BUFFER_SIZE)
        len = BUFFER_SIZE - *offset;

    bytes_read = len - copy_to_user(user_buffer, device_buffer + *offset, len);
    *offset += bytes_read;
    return bytes_read;
}

static ssize_t device_write(struct file *file, const char __user *user_buffer, size_t len, loff_t *offset)
{
    ssize_t bytes_written;

    if (*offset >= BUFFER_SIZE)
        return -ENOSPC;
    if (*offset + len > BUFFER_SIZE)
        len = BUFFER_SIZE - *offset;

    bytes_written = len - copy_from_user(device_buffer + *offset, user_buffer, len);
    *offset += bytes_written;
    return bytes_written;
}

static int device_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release,
};

static int __init uart_init(void)
{
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0)
    {
        printk(KERN_ALERT "Failed to register a major number\n");
        return major_number;
    }

    device_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(device_class))
    {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(device_class);
    }

    if (device_create(device_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME) == NULL)
    {
        class_destroy(device_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return -1;
    }

    cdev_init(&uart_cdev, &fops);
    if (cdev_add(&uart_cdev, MKDEV(major_number, 0), 1) == -1)
    {
        device_destroy(device_class, MKDEV(major_number, 0));
        class_destroy(device_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to add cdev\n");
        return -1;
    }

    printk(KERN_INFO "UART emulator device created successfully\n");
    return 0;
}

static void __exit uart_exit(void)
{
    cdev_del(&uart_cdev);
    device_destroy(device_class, MKDEV(major_number, 0));
    class_destroy(device_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "UART emulator device removed\n");
}

module_init(uart_init);
module_exit(uart_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("UART Emulator Character Device");
MODULE_VERSION("1.0");
