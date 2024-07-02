#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#define DEVICE_NAME "dummy_device"

static struct platform_device dummy_device = {
    .name = DEVICE_NAME,
    .id = -1,
};

static int __init dummy_device_init(void)
{
    int ret;

    ret = platform_device_register(&dummy_device);
    if (ret)
    {
        printk(KERN_ALERT "Failed to register dummy device\n");
        return ret;
    }

    printk(KERN_INFO "Dummy device registered\n");
    return 0;
}

static void __exit dummy_device_exit(void)
{
    platform_device_unregister(&dummy_device);
    printk(KERN_INFO "Dummy device unregistered\n");
}

module_init(dummy_device_init);
module_exit(dummy_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A dummy platform device");
MODULE_VERSION("0.1");
