#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define SWITCH_OFFSET 0x40
#define LED_OFFSET 0x00
#define KEY_OFFSET 0x50
#define KEY_MASK_OFFSET 0x58
#define KEY_EDGE_OFFSET 0x5C

struct priv {
    void __iomem *base_addr;
    int irq;
};

static irqreturn_t irq_handler(int irq, void *data) {
    struct priv *priv = data;
    uint32_t key_status = ioread32(priv->base_addr + KEY_EDGE_OFFSET);

    // KEY0
    if (key_status & 0x1) {
        uint32_t switch_val = ioread32(priv->base_addr + SWITCH_OFFSET);
        iowrite32(switch_val, priv->base_addr + LED_OFFSET);
    }

    // KEY1
    if (key_status & 0x2) {
        uint32_t led_val = ioread32(priv->base_addr + LED_OFFSET);
        iowrite32(led_val >> 1, priv->base_addr + LED_OFFSET);
    }

    // KEY2
    if (key_status & 0x4) {
        uint32_t led_val = ioread32(priv->base_addr + LED_OFFSET);
        iowrite32(led_val << 1, priv->base_addr + LED_OFFSET);
    }

    // Clear interrupt
    iowrite32(0xF, priv->base_addr + KEY_EDGE_OFFSET);

    return IRQ_HANDLED;
}

static int switch_copy_probe(struct platform_device *pdev) {
    struct priv *priv;
    struct resource *res;

    // Check if the device tree node is present
    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv) {
        return -ENOMEM;
    }

    // Request memory region
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    priv->base_addr = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(priv->base_addr)) {
        return PTR_ERR
    }

	// Check if the base address is valid
    (priv->base_addr);

	// Get IRQ number
    priv->irq = platform_get_irq(pdev, 0);
    if (priv->irq < 0) {
        return priv->irq;
    }

	// Request IRQ
    if (devm_request_irq(&pdev->dev, priv->irq, irq_handler, 0, "switch_copy_irq", priv)) {
        return -EBUSY;
    }

    // Enable interrupts for keys
    iowrite32(0xF, priv->base_addr + KEY_MASK_OFFSET);

	// Set the edge trigger for keys
    platform_set_drvdata(pdev, priv);
    return 0;
}

static int switch_copy_remove(struct platform_device *pdev) {
    struct priv *priv = platform_get_drvdata(pdev);

    // Disable LEDs and interrupts
    iowrite32(0x0, priv->base_addr + LED_OFFSET);
    iowrite32(0x0, priv->base_addr + KEY_MASK_OFFSET);

    return 0;
}

static const struct of_device_id switch_copy_driver_id[] = {
    {.compatible = "drv2025"},
    {/* END */},
};

MODULE_DEVICE_TABLE(of, switch_copy_driver_id);

static struct platform_driver switch_copy_driver = {
    .driver =
        {
            .name = "drv-lab4",
            .owner = THIS_MODULE,
            .of_match_table = of_match_ptr(switch_copy_driver_id),
        },
    .probe = switch_copy_probe,
    .remove = switch_copy_remove,
};

module_platform_driver(switch_copy_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("REDS");
MODULE_DESCRIPTION("Introduction to the interrupt and platform drivers");
