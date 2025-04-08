#include "accumulate.h"

#include <linux/cdev.h>   /* Needed for cdev utilities */
#include <linux/device.h> /* Needed for device creation */
#include <linux/fs.h>     /* Needed for file_operations */
#include <linux/init.h>   /* Needed for the macros */
#include <linux/kernel.h> /* Needed for KERN_INFO */
#include <linux/module.h> /* Needed by all modules */
#include <linux/slab.h>   /* Needed for kmalloc */
#include <linux/string.h>
#include <linux/uaccess.h> /* copy_(to|from)_user */

#define MAJOR_NUM 97
#define DEVICE_NAME "accumulate"

#define MAX_NB_VALUE 256

static uint64_t accumulate_value;
static int operation = OP_ADD;

static dev_t dev_number;               // Device number (major + minor)
static struct cdev accumulate_cdev;    // Character device structure
static struct class *accumulate_class; // Device class

static ssize_t accumulate_read(struct file *filp, char __user *buf,
                               size_t count, loff_t *ppos) {
    size_t to_copy;
    int nb_char;

    // Check if the file position is valid
    if (buf == NULL || count == 0) {
        return -EINVAL;
    }

    nb_char = snprintf(NULL, 0, "%llu\n", accumulate_value);

    // Check if the buffer is large enough
    if (*ppos >= nb_char) {
        return 0;
    }

    to_copy = min(count, sizeof(accumulate_value));

    // Copy the value to user space
    if (copy_to_user(buf, &accumulate_value, to_copy)) {
        return -EFAULT;
    }

    // Reset file position to allow multiple reads
    *ppos = 0;

    return to_copy;
}

static ssize_t accumulate_write(struct file *filp, const char __user *buf,
                                size_t count, loff_t *ppos) {
    uint64_t value;

	// Check if the buffer is valid
    if (count < sizeof(value)) {
        return -EINVAL;
    }

	// Copy the value from user space
    if (copy_from_user(&value, buf, sizeof(value))) {
        return -EFAULT;
    }

	// Switch based on the operation
    switch (operation) {
        case OP_ADD:
            accumulate_value += value;
            break;

        case OP_MULTIPLY:
            accumulate_value *= value;
            break;

        default:
            return -EINVAL;
    }

    return sizeof(value);
}

static long accumulate_ioctl(struct file *filep, unsigned int cmd,
                             unsigned long arg) {
    switch (cmd) {
        case ACCUMULATE_CMD_RESET:
            accumulate_value = 0;
            break;

        case ACCUMULATE_CMD_CHANGE_OP:
            if (arg != OP_ADD && arg != OP_MULTIPLY) {
                return -EINVAL;
            }
            operation = arg;
            break;

        default:
            return -EINVAL;
    }
    return 0;
}

static const struct file_operations accumulate_fops = {
    .owner = THIS_MODULE,
    .read = accumulate_read,
    .write = accumulate_write,
    .unlocked_ioctl = accumulate_ioctl,
};

static int __init accumulate_init(void) {
    int ret;

    /* Allocate device number dynamically */
    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("Failed to allocate device number\n");
        return ret;
    }

    /* Initialize and add the character device */
    cdev_init(&accumulate_cdev, &accumulate_fops);
    accumulate_cdev.owner = THIS_MODULE;
    ret = cdev_add(&accumulate_cdev, dev_number, 1);
    if (ret < 0) {
        pr_err("Failed to add cdev\n");
        unregister_chrdev_region(dev_number, 1);
        return ret;
    }

    /* Create device class */
    accumulate_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(accumulate_class)) {
        pr_err("Failed to create class\n");
        cdev_del(&accumulate_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(accumulate_class);
    }

    /* Create device file */
    if (device_create(accumulate_class, NULL, dev_number, NULL, DEVICE_NAME) ==
        NULL) {
        pr_err("Failed to create device\n");
        class_destroy(accumulate_class);
        cdev_del(&accumulate_cdev);
        unregister_chrdev_region(dev_number, 1);
        return -1;
    }

    accumulate_value = 0;

    pr_info("Accumulate ready!\n");
    pr_info("Device created at /dev/%s\n", DEVICE_NAME);
    pr_info("ioctl ACCUMULATE_CMD_RESET: %lu\n",
            (unsigned long)ACCUMULATE_CMD_RESET);
    pr_info("ioctl ACCUMULATE_CMD_CHANGE_OP: %lu\n",
            (unsigned long)ACCUMULATE_CMD_CHANGE_OP);

    return 0;
}

static void __exit accumulate_exit(void) {
    // Remove device file
    device_destroy(accumulate_class, dev_number);

    // Destroy device class
    class_destroy(accumulate_class);

    // Remove character device
    cdev_del(&accumulate_cdev);

    // Free device number
    unregister_chrdev_region(dev_number, 1);

    pr_info("Accumulate done!\n");
}

MODULE_AUTHOR("REDS");
MODULE_LICENSE("GPL");

module_init(accumulate_init);
module_exit(accumulate_exit);
