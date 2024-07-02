#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/platform_device.h>

#define DEVICE_NAME "dummy_device"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A dummy device driver with poll support");
MODULE_VERSION("0.1");

#define QUEUE_SIZE 10
#define FRAME_SIZE 256

static char queue[QUEUE_SIZE][FRAME_SIZE];
static int front = 0;
static int rear = -1;
static int count = 0;

static struct mutex queue_lock;
static DECLARE_WAIT_QUEUE_HEAD(queue_wq);

// Function to enqueue data
static int enqueue_frame(const char *data, size_t len)
{
    if (count == QUEUE_SIZE)
    {
        return -ENOMEM; // Queue is full
    }

    rear = (rear + 1) % QUEUE_SIZE;
    strncpy(queue[rear], data, len);
    count++;
    wake_up_interruptible(&queue_wq);
    return 0;
}

// Function to dequeue data
static int dequeue_frame(char *buffer)
{
    if (count == 0)
    {
        return -ENODATA; // Queue is empty
    }

    strncpy(buffer, queue[front], FRAME_SIZE);
    front = (front + 1) % QUEUE_SIZE;
    count--;
    return 0;
}

// Driver's write interface
ssize_t dummy_driver_write(const char *buffer, size_t len)
{
    int ret;

    mutex_lock(&queue_lock);
    ret = enqueue_frame(buffer, len);
    mutex_unlock(&queue_lock);

    return ret;
}
EXPORT_SYMBOL(dummy_driver_write);

// Driver's read interface
ssize_t dummy_driver_read(char *buffer)
{
    int ret;

    mutex_lock(&queue_lock);
    ret = dequeue_frame(buffer);
    mutex_unlock(&queue_lock);

    return ret;
}
EXPORT_SYMBOL(dummy_driver_read);

// Driver's poll interface
unsigned int dummy_driver_poll(struct file *file, poll_table *wait)
{
    unsigned int mask = 0;

    poll_wait(file, &queue_wq, wait);

    mutex_lock(&queue_lock);
    if (count > 0)
    {
        // Readable
        mask |= POLLIN | POLLRDNORM;
    }
    mutex_unlock(&queue_lock);

    return mask;
}
EXPORT_SYMBOL(dummy_driver_poll);

// Platform driver probe function
static int dummy_probe(struct platform_device *pdev)
{
    printk(KERN_INFO "Dummy driver probed\n");
    return 0;
}

// Platform driver remove function
static int dummy_remove(struct platform_device *pdev)
{
    printk(KERN_INFO "Dummy driver removed\n");
    return 0;
}

// static struct device_driver dummy_driver;

static struct platform_driver dummy_driver = {
    .probe = dummy_probe,
    .remove = dummy_remove,
    .driver = {
        .name = DEVICE_NAME,
        .owner = THIS_MODULE,
    },
};

// Driver registration function
static int __init dummy_driver_init(void)
{
    int ret;

    mutex_init(&queue_lock);

    // dummy_driver.name = DEVICE_NAME;
    // dummy_driver.bus = &platform_bus_type;
    // dummy_driver.owner = THIS_MODULE;
    // ret = driver_register(&dummy_driver);

    ret = platform_driver_register(&dummy_driver);
    if (ret < 0)
    {
        printk(KERN_ALERT "Failed to register dummy driver\n");
        return ret;
    }

    printk(KERN_INFO "Dummy driver registered\n");
    return 0;
}

// Driver unregistration function
static void __exit dummy_driver_exit(void)
{
    // driver_unregister(&dummy_driver);
    platform_driver_unregister(&dummy_driver);
    printk(KERN_INFO "Dummy driver unregistered\n");
}

module_init(dummy_driver_init);
module_exit(dummy_driver_exit);
