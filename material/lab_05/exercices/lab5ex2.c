#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/uaccess.h>

#define MAX_SEQUENCES 16
#define LED_COUNT 10
#define DEFAULT_INTERVAL_MS 1000  // Default interval in milliseconds
#define LED_OFFSET 0x00
#define LAB5EX_MAJOR 240  // Use a static major number
#define LAB5EX_NAME "lab5ex2"

static struct task_struct *chaser_thread;
static DECLARE_KFIFO(sequence_fifo, char, MAX_SEQUENCES);
static DEFINE_MUTEX(fifo_lock);
static struct miscdevice chaser_miscdev;

static int interval_ms = DEFAULT_INTERVAL_MS;
module_param(interval_ms, int, 0644);
MODULE_PARM_DESC(interval_ms, "Interval in milliseconds between LED animations");

static void __iomem *led_base_addr;

static int chaser_open(struct inode *inode, struct file *file) { return 0; }

// Write handler: accepts "up" or "down" commands and queues them
static ssize_t chaser_write(struct file *file, const char __user *buf, size_t len, loff_t *off) {
    char kbuf[8];
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

// No read functionality
static ssize_t chaser_read(struct file *file, char __user *buf, size_t len, loff_t *off) {
    return 0;
}

static int chaser_release(struct inode *inode, struct file *file) { return 0; }

// File operations for misc device
static const struct file_operations lab5ex2_fops = {
    .owner = THIS_MODULE,
    .write = chaser_write,
};

static struct timer_list led_timer;
static struct completion seq_done;
static int seq_direction = 0;  // 0: up, 1: down
static int seq_index = 0;
static int seq_active = 0;

static void start_sequence(int direction) {
    seq_direction = direction;
    seq_index = 0;
    seq_active = 1;
    reinit_completion(&seq_done);
    mod_timer(&led_timer, jiffies + msecs_to_jiffies(interval_ms));
}

static int chaser_thread_fn(void *data) {
    char command;

    printk(KERN_INFO "Chaser thread started\n");

    while (!kthread_should_stop()) {
        mutex_lock(&fifo_lock);
        if (kfifo_get(&sequence_fifo, &command)) {
            mutex_unlock(&fifo_lock);

            // Start the sequence using the timer
            if (command == 'u')
                start_sequence(0);
            else
                start_sequence(1);

            // Wait for the sequence to finish
            wait_for_completion(&seq_done);
        } else {
            mutex_unlock(&fifo_lock);
            msleep(100);  // Wait for new commands
        }
    }

    printk(KERN_INFO "Chaser thread stopped\n");

    return 0;
}

// Set or clear a specific LED
static void control_leds(int led, bool state) {
    uint32_t led_val = ioread32(led_base_addr);

    if (state)
        led_val |= (1 << led);  // Turn on the LED
    else
        led_val &= ~(1 << led);  // Turn off the LED

    iowrite32(led_val, led_base_addr);
}

// Timer callback for LED animation
static void led_timer_callback(struct timer_list *t) {
    // Turn off previous LED
    if (seq_direction == 0) {  // up
        if (seq_index > 0)
            control_leds(seq_index - 1, false);
    } else {  // down
        if (seq_index > 0)
            control_leds(LED_COUNT - seq_index, false);
    }

    // Turn on current LED if in range
    if (seq_index < LED_COUNT) {
        if (seq_direction == 0)
            control_leds(seq_index, true);
        else
            control_leds(LED_COUNT - 1 - seq_index, true);
        seq_index++;
        mod_timer(&led_timer, jiffies + msecs_to_jiffies(interval_ms));
        return;
    }

    // Sequence finished, turn off last LED
    if (seq_direction == 0)
        control_leds(LED_COUNT - 1, false);
    else
        control_leds(0, false);

    seq_active = 0;
    complete(&seq_done);
    return;
}

static const struct of_device_id drv_lab5_1_id[] = {
    {.compatible = "drv2025"},
    {/* END */},
};

MODULE_DEVICE_TABLE(of, drv_lab5_1_id);

static int drv_lab5_probe(struct platform_device *pdev) {
    struct resource *res;
    int ret;

    // Map device memory
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        pr_err("lab5ex2: Failed to get resource for drv2025\n");
        return -ENODEV;
    }
    led_base_addr = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(led_base_addr)) {
        pr_err("lab5ex2: Failed to map LED memory\n");
        return PTR_ERR(led_base_addr);
    }

    INIT_KFIFO(sequence_fifo);

    // Init completion and timer
    init_completion(&seq_done);
    timer_setup(&led_timer, led_timer_callback, 0);

    // Register misc device
    chaser_miscdev.minor = MISC_DYNAMIC_MINOR;
    chaser_miscdev.name = LAB5EX_NAME;
    chaser_miscdev.fops = &lab5ex2_fops;
    chaser_miscdev.mode = 0666;
    ret = misc_register(&chaser_miscdev);
    if (ret) {
        pr_err("lab5ex2: Failed to register misc device\n");
        return ret;
    }

    // Start the kthread
    chaser_thread = kthread_run(chaser_thread_fn, NULL, "chaser_thread");
    if (IS_ERR(chaser_thread)) {
        pr_err("lab5ex2: Failed to create kthread\n");
        misc_deregister(&chaser_miscdev);
        return PTR_ERR(chaser_thread);
    }

    pr_info("lab5ex2: Driver successfully initialized!\n");
    return 0;
}

static int drv_lab5_remove(struct platform_device *pdev) {
    kthread_stop(chaser_thread);
    del_timer_sync(&led_timer);
    misc_deregister(&chaser_miscdev);
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
