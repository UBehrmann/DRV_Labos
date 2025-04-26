// SPDX-License-Identifier: GPL-2.0
/*
 * Parrot file
 */

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#define MAJOR_NUM 98
#define MAJMIN MKDEV(MAJOR_NUM, 0)
#define DEVICE_NAME "parrot"

#define INITIAL_CAPACITY 8
#define MAX_CAPACITY 1024

static struct cdev cdev;
static struct class *cl;
static char *buffer;
static size_t buffer_size = 0;  // Number of bytes written to the buffer
static size_t buffer_capacity = INITIAL_CAPACITY;

/**
 * @brief Read back previously written data in the internal buffer.
 *
 * @param filp pointer to the file descriptor in use
 * @param buf destination buffer in user space
 * @param count maximum number of data to read
 * @param ppos current position in file from which data will be read
 *              will be updated to new location
 *
 * @return Actual number of bytes read from internal buffer,
 *         or a negative error code
 */
static ssize_t parrot_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos) {
    // No more data to read
    if (*ppos >= buffer_size) {
        return 0;
    }

    // Adjust count to available data
    if (*ppos + count > buffer_size) {
        count = buffer_size - *ppos;
    }

	// Copy data from kernel space to user space
    if (copy_to_user(buf, buffer + *ppos, count)) {
        return -EFAULT;
    }

    *ppos += count;
    return count;
}

/**
 * @brief Write data to the internal buffer
 *
 * @param filp pointer to the file descriptor in use
 * @param buf source buffer in user space
 * @param count number of data to write in the buffer
 * @param ppos current position in file to which data will be written
 *              will be updated to new location
 *
 * @return Actual number of bytes writen to internal buffer,
 *         or a negative error code
 */
static ssize_t parrot_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos) {
    size_t new_size;
    char *new_buffer;

    // Writing beyond max capacity
    if (*ppos >= MAX_CAPACITY) {
        return -EFBIG;
    }

    // Adjust count to fit max capacity
    if (*ppos + count > MAX_CAPACITY) {
        count = MAX_CAPACITY - *ppos;
    }

    new_size = *ppos + count;
    if (new_size > buffer_capacity) {
        size_t new_capacity = buffer_capacity;

        // Double the capacity until it is large enough to hold the new size
        // Same as the allocation strategy for vectors in C++
        while (new_capacity < new_size && new_capacity < MAX_CAPACITY) {
            new_capacity *= 2;
        }

        // Ensure we do not exceed the maximum capacity
        if (new_capacity > MAX_CAPACITY) {
            new_capacity = MAX_CAPACITY;
        }

        // Allocate new memory for the buffer
        new_buffer = krealloc(buffer, new_capacity, GFP_KERNEL);
        if (!new_buffer) {
            return -ENOMEM;
        }

        buffer = new_buffer;
        buffer_capacity = new_capacity;
    }

    // Copy data from user space to kernel space
    if (copy_from_user(buffer + *ppos, buf, count)) return -EFAULT;

    *ppos += count;

    // Update the size of the buffer if necessary
    if (new_size > buffer_size) buffer_size = new_size;

    return count;
}

/**
 * @brief uevent callback to set the permission on the device file
 *
 * @param dev pointer to the device
 * @param env ueven environnement corresponding to the device
 */
static int parrot_uevent(struct device *dev, struct kobj_uevent_env *env) {
    // Set the permissions of the device file
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}

static const struct file_operations parrot_fops = {
    .owner = THIS_MODULE,
    .read = parrot_read,
    .write = parrot_write,
    .llseek = default_llseek,  // Use default to enable seeking to 0
};

static int __init parrot_init(void) {
    struct device *clsdev;
    int err;

    // Register the device
    err = register_chrdev_region(MAJMIN, 1, DEVICE_NAME);
    if (err != 0) {
        pr_err("Parrot: Registering char device failed (%d)\n", err);
        return err;
    }

    cl = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(cl)) {
        err = PTR_ERR(cl);
        pr_err("Parrot: Error creating class (%d)\n", err);
        goto err_class_create;
    }
    cl->dev_uevent = parrot_uevent;

    clsdev = device_create(cl, NULL, MAJMIN, NULL, DEVICE_NAME);
    if (IS_ERR(clsdev)) {
        err = PTR_ERR(clsdev);
        pr_err("Parrot: Error creating device (%d)\n", err);
        goto err_device_create;
    }

    cdev_init(&cdev, &parrot_fops);
    err = cdev_add(&cdev, MAJMIN, 1);
    if (err < 0) {
        pr_err("Parrot: Adding char device failed (%d)\n", err);
        goto err_cdev_add;
    }

    buffer = kmalloc(INITIAL_CAPACITY, GFP_KERNEL);
    if (!buffer) {
        pr_err("Parrot: Failed to allocate initial buffer\n");
        return -ENOMEM;
    }

    buffer_size = 0;
    buffer_capacity = INITIAL_CAPACITY;

    pr_info("Parrot ready!\n");

    return 0;

err_cdev_add:
    device_destroy(cl, MAJMIN);
err_device_create:
    class_destroy(cl);
err_class_create:
    unregister_chrdev_region(MAJMIN, 1);
    kfree(buffer);
    return err;
}

static void __exit parrot_exit(void) {
    // Unregister the device
    cdev_del(&cdev);
    device_destroy(cl, MAJMIN);
    class_destroy(cl);
    unregister_chrdev_region(MAJMIN, 1);
    kfree(buffer);

    pr_info("Parrot done!\n");
}

MODULE_AUTHOR("REDS");
MODULE_LICENSE("GPL");

module_init(parrot_init);
module_exit(parrot_exit);
