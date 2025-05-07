#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>

#define MAX_SEQUENCES 16
#define LED_COUNT 10
#define DEFAULT_INTERVAL_MS 1000 // Default interval in milliseconds
#define LED_OFFSET 0x00
#define LAB5EX1_MAJOR 240  // Use a static major number
#define LAB5EX1_NAME "lab5ex1"

static struct task_struct *chaser_thread;
static DECLARE_KFIFO(sequence_fifo, char, MAX_SEQUENCES);
static DEFINE_MUTEX(fifo_lock);
static struct cdev lab5ex1_cdev;
static struct class *lab5ex1_class;

static int interval_ms = DEFAULT_INTERVAL_MS;
module_param(interval_ms, int, 0644);
MODULE_PARM_DESC(interval_ms, "Interval in milliseconds between LED animations");

static void __iomem *led_base_addr;

static int chaser_open(struct inode *inode, struct file *file) { return 0; }

// Update chaser_write to handle file writes
static ssize_t chaser_write(struct file *file, const char __user *buf, size_t len, loff_t *off) {
    char kbuf[8];  // Buffer to hold the command
    char cmd;

    // Ensure the input length is valid
    if (len >= sizeof(kbuf)) return -EINVAL;

    // Copy data from user space to kernel space
    if (copy_from_user(kbuf, buf, len)) return -EFAULT;
    kbuf[len] = '\0';  // Null-terminate the string

    // Remove trailing newline if present
    if (kbuf[len - 1] == '\n') kbuf[len - 1] = '\0';

    // Identify the command
    if (strcmp(kbuf, "up") == 0) {
        cmd = 'u';  // Use 'u' for "up"
    } else if (strcmp(kbuf, "down") == 0) {
        cmd = 'd';  // Use 'd' for "down"
    } else {
        pr_err("Chaser: Invalid command \"%s\"\n", kbuf);
        return -EINVAL;
    }

    // Add the command to the FIFO queue
    mutex_lock(&fifo_lock);
    if (!kfifo_put(&sequence_fifo, cmd)) {
        pr_warn("Chaser: FIFO is full, command \"%s\" not added\n", kbuf);
        mutex_unlock(&fifo_lock);
        return -ENOSPC;
    }
    mutex_unlock(&fifo_lock);

    pr_info("Chaser: Command \"%s\" added to FIFO\n", kbuf);
    return len;
}

static ssize_t chaser_read(struct file *file, char __user *buf, size_t len, loff_t *off) {
    return 0;  // No read functionality for now
}

static int chaser_release(struct inode *inode, struct file *file) { return 0; }

// Add file operations for the character device
static const struct file_operations lab5ex1_fops = {
    .owner = THIS_MODULE,
    .write = chaser_write,
};

static void control_leds(int led, bool state) {
    uint32_t led_val = ioread32(led_base_addr);

    if (state)
        led_val |= (1 << led);  // Turn on the LED
    else
        led_val &= ~(1 << led);  // Turn off the LED

    iowrite32(led_val, led_base_addr);
}

static void run_sequence(char direction) {
    int i;

    if (direction == 'u') {  // Up sequence
        for (i = 0; i < LED_COUNT; i++) {
            control_leds(i, true);
            msleep(interval_ms);
            control_leds(i, false);
        }
    } else if (direction == 'd') {  // Down sequence
        for (i = LED_COUNT - 1; i >= 0; i--) {
            control_leds(i, true);
            msleep(interval_ms);
            control_leds(i, false);
        }
    }
}

static int chaser_thread_fn(void *data) {
    char command;

    printk(KERN_INFO "Chaser thread started\n");

    while (!kthread_should_stop()) {
        mutex_lock(&fifo_lock);
        if (kfifo_get(&sequence_fifo, &command)) {
            mutex_unlock(&fifo_lock);
            run_sequence(command);
        } else {
            mutex_unlock(&fifo_lock);
            msleep(100);  // Wait for new commands
        }
    }

    printk(KERN_INFO "Chaser thread stopped\n");

    return 0;
}

static const struct of_device_id drv_lab5_1_id[] = {
    {.compatible = "drv2025"},
    {/* END */},
};

MODULE_DEVICE_TABLE(of, drv_lab5_1_id);

static int drv_lab5_probe(struct platform_device *pdev) {
    struct resource *res;
    int ret;

    pr_info("Chaser module probe\n");

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        pr_err("Failed to get resource for drv2025\n");
        return -ENODEV;
    }

    led_base_addr = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(led_base_addr)) {
        pr_err("Failed to map LED memory\n");
        return PTR_ERR(led_base_addr);
    }

    INIT_KFIFO(sequence_fifo);

    chaser_thread = kthread_run(chaser_thread_fn, NULL, "chaser_thread");
    if (IS_ERR(chaser_thread)) {
        pr_err("Failed to create kthread\n");
        return PTR_ERR(chaser_thread);
    }

    // Register character device
    ret = register_chrdev_region(MKDEV(LAB5EX1_MAJOR, 0), 1, LAB5EX1_NAME);
    if (ret < 0) {
        pr_err("Failed to register char device\n");
        return ret;
    }

    cdev_init(&lab5ex1_cdev, &lab5ex1_fops);
    ret = cdev_add(&lab5ex1_cdev, MKDEV(LAB5EX1_MAJOR, 0), 1);
    if (ret < 0) {
        unregister_chrdev_region(MKDEV(LAB5EX1_MAJOR, 0), 1);
        pr_err("Failed to add char device\n");
        return ret;
    }

    lab5ex1_class = class_create(THIS_MODULE, LAB5EX1_NAME);
    if (IS_ERR(lab5ex1_class)) {
        cdev_del(&lab5ex1_cdev);
        unregister_chrdev_region(MKDEV(LAB5EX1_MAJOR, 0), 1);
        return PTR_ERR(lab5ex1_class);
    }

    if (!device_create(lab5ex1_class, NULL, MKDEV(LAB5EX1_MAJOR, 0), NULL, LAB5EX1_NAME)) {
        class_destroy(lab5ex1_class);
        cdev_del(&lab5ex1_cdev);
        unregister_chrdev_region(MKDEV(LAB5EX1_MAJOR, 0), 1);
        return -ENOMEM;
    }

    return 0;
}

static int drv_lab5_remove(struct platform_device *pdev) {
    kthread_stop(chaser_thread);

    device_destroy(lab5ex1_class, MKDEV(LAB5EX1_MAJOR, 0));
    class_destroy(lab5ex1_class);
    cdev_del(&lab5ex1_cdev);
    unregister_chrdev_region(MKDEV(LAB5EX1_MAJOR, 0), 1);

    pr_info("Chaser module removed\n");
    return 0;
}

static struct platform_driver drv_lab5_driver = {
    .driver =
        {
            .name = "drv_lab5",
            .owner = THIS_MODULE,
            .of_match_table = of_match_ptr(drv_lab5_1_id),
        },
    .probe = drv_lab5_probe,
    .remove = drv_lab5_remove,
};

module_platform_driver(drv_lab5_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("REDS");
MODULE_DESCRIPTION("Chaser LED control module");
