#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#include "device_driver_dummy.h"

#define DEVICE_NAME "dummy_chardev"
#define CLASS_NAME "dummy"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A character device driver that interfaces with the dummy driver");
MODULE_VERSION("0.1");

static int majorNumber;
static struct class *dummyClass = NULL;
static struct device *dummyDevice = NULL;
static struct cdev cdev;

static int dev_open(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "DummyCharDev: Device has been opened\n");
    return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
    char frame[FRAME_SIZE];
    int ret = dummy_driver_read(frame);

    if (ret == 0)
    {
        if (copy_to_user(buffer, frame, FRAME_SIZE))
        {
            return -EFAULT;
        }
        return FRAME_SIZE;
    }
    return ret;
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
    char frame[FRAME_SIZE];

    if (len > FRAME_SIZE)
    {
        return -EINVAL; // Input too large
    }

    if (copy_from_user(frame, buffer, len))
    {
        return -EFAULT;
    }

    return dummy_driver_write(frame, len);
}

static int dev_release(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "DummyCharDev: Device successfully closed\n");
    return 0;
}

static unsigned int dev_poll(struct file *file, poll_table *wait)
{
    return dummy_driver_poll(file, wait);
}

static struct file_operations fops = {
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
    .poll = dev_poll,
};

static int __init chardev_init(void)
{
    printk(KERN_INFO "DummyCharDev: Initializing the DummyCharDev\n");

    // Dynamically allocate a major number for the device
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0)
    {
        printk(KERN_ALERT "DummyCharDev failed to register a major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "DummyCharDev: registered correctly with major number %d\n", majorNumber);

// Register the device class
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0)
    dummyClass = class_create(THIS_MODULE, DEVICE_NAME);
#else
    dummyClass = class_create(DEVICE_NAME);
#endif

    if (IS_ERR(dummyClass))
    {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(dummyClass);
    }
    printk(KERN_INFO "DummyCharDev: device class registered correctly\n");

    // Register the device driver
    dummyDevice = device_create(dummyClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(dummyDevice))
    {
        class_destroy(dummyClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(dummyDevice);
    }
    printk(KERN_INFO "DummyCharDev: device class created correctly\n");

    // Initialize the cdev structure
    cdev_init(&cdev, &fops);
    cdev_add(&cdev, MKDEV(majorNumber, 0), 1);

    return 0;
}

static void __exit chardev_exit(void)
{
    cdev_del(&cdev);
    device_destroy(dummyClass, MKDEV(majorNumber, 0)); // remove the device
    class_unregister(dummyClass);                      // unregister the device class
    class_destroy(dummyClass);                         // remove the device class
    unregister_chrdev(majorNumber, DEVICE_NAME);       // unregister the major number
    printk(KERN_INFO "DummyCharDev: Goodbye from the LKM!\n");
}

module_init(chardev_init);
module_exit(chardev_exit);
