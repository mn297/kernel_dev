#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "char_device"
#define EXAMPLE_MSG "Hello, World!\n"
#define MSG_BUFFER_LEN 15

static int major_num;
static char msg_buffer[MSG_BUFFER_LEN];
static int msg_len;

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release,
};

static int __init char_device_init(void)
{
    major_num = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_num < 0)
    {
        printk(KERN_ALERT "Registering char device failed with %d\n", major_num);
        return major_num;
    }
    printk(KERN_INFO "Registered char device with major number %d\n", major_num);
    return 0;
}

static void __exit char_device_exit(void)
{
    unregister_chrdev(major_num, DEVICE_NAME);
    printk(KERN_INFO "Unregistered char device\n");
}

static int device_open(struct inode *inode, struct file *file)
{
    snprintf(msg_buffer, MSG_BUFFER_LEN, EXAMPLE_MSG);
    msg_len = strlen(msg_buffer);
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t device_read(struct file *file, char *buffer, size_t len, loff_t *offset)
{
    int bytes_read = 0;
    if (*offset >= msg_len)
    {
        return 0;
    }
    while (len && *offset < msg_len)
    {
        put_user(msg_buffer[*offset], buffer++);
        len--;
        bytes_read++;
        (*offset)++;
    }
    return bytes_read;
}

static ssize_t device_write(struct file *file, const char *buffer, size_t len, loff_t *offset)
{
    printk(KERN_ALERT "This operation is not supported.\n");
    return -EINVAL;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple character device driver");

module_init(char_device_init);
module_exit(char_device_exit);
