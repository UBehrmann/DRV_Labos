#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/slab.h>

// Basic register definitions for ADXL345 accelerometer
#define ADXL345_DEVID_REG 0x00
#define ADXL345_DEVID_VAL 0xE5
#define ADXL345_DATA_FORMAT_REG 0x31
#define ADXL345_DATA_FORMAT_4G 0x01  // +/-4g 
// 0x00 = +/-2g
// 0x01 = +/-4g
// 0x02 = +/-8g
// 0x03 = +/-16g
#define ADXL345_POWER_CTL_REG 0x2D
#define ADXL345_POWER_CTL_MEASURE 0x08
#define ADXL345_POWER_CTL_STANDBY 0x00
#define ADXL345_DATAX0_REG 0x32
#define ADXL345_DATA_LEN 6  // 2 octets par axe (X, Y, Z)

// Tap/double tap registers
#define ADXL345_THRESH_TAP_REG 0x1D
#define ADXL345_DUR_REG        0x21
#define ADXL345_LATENT_REG     0x22
#define ADXL345_WINDOW_REG     0x23
#define ADXL345_TAP_AXES_REG   0x2A
#define ADXL345_ACT_TAP_STATUS_REG 0x2B
#define ADXL345_INT_ENABLE_REG 0x2E
#define ADXL345_INT_MAP_REG    0x2F
#define ADXL345_INT_SOURCE_REG 0x30

// Reasonable default values
#define ADXL345_THRESH_TAP_VAL 0x10   // ~1g (0.0625g/LSB)
#define ADXL345_DUR_VAL        0x10   // ~10ms (625us/LSB)
#define ADXL345_LATENT_VAL     0x30   // ~48ms (1.25ms/LSB)
#define ADXL345_WINDOW_VAL     0xA0   // ~200ms (1.25ms/LSB)
#define ADXL345_TAP_AXES_X     0x01   // Enable X axis tap
#define ADXL345_TAP_AXES_Y     0x02   // Enable Y axis tap
#define ADXL345_TAP_AXES_Z     0x04   // Enable Z axis tap
#define ADXL345_INT_SINGLE_TAP 0x40   // Enable single tap detection (Bit 6)
#define ADXL345_INT_DOUBLE_TAP 0x20   // Enable double tap detection (Bit 5)

#define DEVICE_NAME "adxl345"

// ADXL345 register map for reference
// 0x00 - DEVID
// 0x1D - THRESH_TAP
// 0x21 - DUR
// 0x22 - LATENT
// 0x23 - WINDOW
// 0x2A - TAP_AXES
// 0x2B - ACT_TAP_STATUS
// 0x2D - POWER_CTL
// 0x2E - INT_ENABLE
// 0x2F - INT_MAP
// 0x30 - INT_SOURCE
// 0x31 - DATA_FORMAT
// 0x32 - DATAX0
// 0x33 - DATAX1
// 0x34 - DATAY0
// 0x35 - DATAY1
// 0x36 - DATAZ0
// 0x37 - DATAZ1

// Structure pour stocker les données du driver
struct adxl345_data {
    struct i2c_client *client;
    struct miscdevice miscdev;
    int irq_requested;

    // sysfs state
    char tap_axis; // 'x', 'y', or 'z'
    char tap_mode; // 0=off, 1=single, 2=double, 3=both
    atomic_t tap_count;
    struct mutex tap_lock; // protects tap_axis and tap_mode
    struct completion tap_event;
    int tap_wait_busy;
    int last_tap; // 1=single, 2=double
};

// Lit les 3 axes (X, Y, Z) depuis le capteur via I2C
static int adxl345_read_axes(struct i2c_client *client, short *x, short *y, short *z) {
    u8 data[ADXL345_DATA_LEN];
    int ret;

    ret = i2c_smbus_read_i2c_block_data(client, ADXL345_DATAX0_REG, ADXL345_DATA_LEN, data);
    if (ret < 0)
        return ret;

    // Les valeurs sont sur 16 bits, little endian
    *x = (short)((data[1] << 8) | data[0]);
    *y = (short)((data[3] << 8) | data[2]);
    *z = (short)((data[5] << 8) | data[4]);

    return 0;
}

// Convertit la valeur brute en g (résolution 4mg/LSB en mode +/-4g)
static void adxl345_to_g(short val, char *buf) {
    // 1 LSB = 4mg = 0.004g
    int g_int = val * 4 / 1000;
    int g_frac = abs(val * 4 % 1000);
    snprintf(buf, 16, "%+d.%03d", g_int, g_frac);
}

// Lecture du device : retourne la dernière mesure formatée
static ssize_t adxl345_read(struct file *file, char __user *buf, size_t count, loff_t *offset) {
    struct adxl345_data *data = container_of(file->private_data, struct adxl345_data, miscdev);
    short x, y, z;
    char outbuf[192];
    char xbuf[16], ybuf[16], zbuf[16];
    int len, ret;
    int int_source = 0, tap_status = 0;

    if (*offset > 0)
        return 0;

    ret = adxl345_read_axes(data->client, &x, &y, &z);
    if (ret)
        return ret;

    adxl345_to_g(x, xbuf);
    adxl345_to_g(y, ybuf);
    adxl345_to_g(z, zbuf);

    // Read tap/double tap status
    int_source = i2c_smbus_read_byte_data(data->client, ADXL345_INT_SOURCE_REG);
    if (int_source >= 0 && (int_source & (ADXL345_INT_SINGLE_TAP | ADXL345_INT_DOUBLE_TAP))) {
        tap_status = i2c_smbus_read_byte_data(data->client, ADXL345_ACT_TAP_STATUS_REG);
    }

    len = snprintf(outbuf, sizeof(outbuf),
        "X = %s; Y = %s; Z = %s\n\n"
        "INT_SOURCE = 0x%02x; ACT_TAP_STATUS = 0x%02x\n",
        xbuf, ybuf, zbuf,
        int_source, tap_status
    );

    if (len > count)
        len = count;

    if (copy_to_user(buf, outbuf, len))
        return -EFAULT;

    *offset += len;
    return len;
}

// --- SYSFS HELPERS ---

static int adxl345_update_tap_axis(struct adxl345_data *data, char axis)
{
    u8 axes = 0;
    switch (axis) {
        case 'x': axes = ADXL345_TAP_AXES_X; break;
        case 'y': axes = ADXL345_TAP_AXES_Y; break;
        case 'z': axes = ADXL345_TAP_AXES_Z; break;
        default: return -EINVAL;
    }
    return i2c_smbus_write_byte_data(data->client, ADXL345_TAP_AXES_REG, axes);
}

static int adxl345_update_tap_mode(struct adxl345_data *data, char mode)
{
    u8 enable = 0;
    switch (mode) {
        case 0: enable = 0; break; // off
        case 1: enable = ADXL345_INT_SINGLE_TAP; break;
        case 2: enable = ADXL345_INT_DOUBLE_TAP; break;
        case 3: enable = ADXL345_INT_SINGLE_TAP | ADXL345_INT_DOUBLE_TAP; break;
        default: return -EINVAL;
    }
    return i2c_smbus_write_byte_data(data->client, ADXL345_INT_ENABLE_REG, enable);
}

// --- SYSFS ATTRIBUTES ---

static ssize_t tap_axis_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct adxl345_data *data = dev_get_drvdata(dev);
    char axis;
    mutex_lock(&data->tap_lock);
    axis = data->tap_axis;
    mutex_unlock(&data->tap_lock);
    return sysfs_emit(buf, "%c\n", axis);
}

static ssize_t tap_axis_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct adxl345_data *data = dev_get_drvdata(dev);
    char axis;
    int ret = 0;

    if (buf[0] == 'x') axis = 'x';
    else if (buf[0] == 'y') axis = 'y';
    else if (buf[0] == 'z') axis = 'z';
    else return -EINVAL;

    mutex_lock(&data->tap_lock);
    ret = adxl345_update_tap_axis(data, axis);
    if (!ret)
        data->tap_axis = axis;
    mutex_unlock(&data->tap_lock);
    return ret ? ret : count;
}
static DEVICE_ATTR_RW(tap_axis);

static ssize_t tap_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct adxl345_data *data = dev_get_drvdata(dev);
    char mode;
    ssize_t n;
    mutex_lock(&data->tap_lock);
    mode = data->tap_mode;
    mutex_unlock(&data->tap_lock);
    switch (mode) {
        case 0: n = sysfs_emit(buf, "off\n"); break;
        case 1: n = sysfs_emit(buf, "single\n"); break;
        case 2: n = sysfs_emit(buf, "double\n"); break;
        case 3: n = sysfs_emit(buf, "both\n"); break;
        default: n = sysfs_emit(buf, "off\n");
    }
    return n;
}

static ssize_t tap_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct adxl345_data *data = dev_get_drvdata(dev);
    char mode;
    int ret = 0;

    if (sysfs_streq(buf, "off")) mode = 0;
    else if (sysfs_streq(buf, "single")) mode = 1;
    else if (sysfs_streq(buf, "double")) mode = 2;
    else if (sysfs_streq(buf, "both")) mode = 3;
    else return -EINVAL;

    mutex_lock(&data->tap_lock);
    ret = adxl345_update_tap_mode(data, mode);
    if (!ret)
        data->tap_mode = mode;
    mutex_unlock(&data->tap_lock);
    return ret ? ret : count;
}
static DEVICE_ATTR_RW(tap_mode);

static ssize_t tap_count_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct adxl345_data *data = dev_get_drvdata(dev);
    return sysfs_emit(buf, "%d\n", atomic_read(&data->tap_count));
}
static DEVICE_ATTR_RO(tap_count);

static ssize_t tap_wait_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct adxl345_data *data = dev_get_drvdata(dev);
    int last_tap;
    int ret = 0;

    mutex_lock(&data->tap_lock);
    if (data->tap_wait_busy) {
        mutex_unlock(&data->tap_lock);
        return sysfs_emit(buf, "busy\n");
    }
    data->tap_wait_busy = 1;
    reinit_completion(&data->tap_event);
    mutex_unlock(&data->tap_lock);

    // Wait for event (interrupt handler will complete)
    if (wait_for_completion_interruptible(&data->tap_event)) {
        // interrupted
        mutex_lock(&data->tap_lock);
        data->tap_wait_busy = 0;
        mutex_unlock(&data->tap_lock);
        return -ERESTARTSYS;
    }

    mutex_lock(&data->tap_lock);
    last_tap = data->last_tap;
    data->tap_wait_busy = 0;
    mutex_unlock(&data->tap_lock);

    if (last_tap == 1)
        ret = sysfs_emit(buf, "single\n");
    else if (last_tap == 2)
        ret = sysfs_emit(buf, "double\n");
    else
        ret = sysfs_emit(buf, "unknown\n");
    return ret;
}
static DEVICE_ATTR_RO(tap_wait);

// IRQ handler for tap/double tap
static irqreturn_t adxl345_irq_thread(int irq, void *dev_id) {
    struct adxl345_data *data = dev_id;
    struct i2c_client *client = data->client;
    int int_source, tap_status;
    int tap_type = 0;

    // Read INT_SOURCE to acknowledge and determine cause
    int_source = i2c_smbus_read_byte_data(client, ADXL345_INT_SOURCE_REG);
    if (int_source < 0)
        return IRQ_NONE;

    // Only handle tap/double tap interrupts
    if (int_source & (ADXL345_INT_SINGLE_TAP | ADXL345_INT_DOUBLE_TAP)) {
        tap_status = i2c_smbus_read_byte_data(client, ADXL345_ACT_TAP_STATUS_REG);
        if (tap_status < 0)
            return IRQ_NONE;

        if (int_source & ADXL345_INT_SINGLE_TAP) {
            printk(KERN_INFO "adxl345: Single tap detected (status=0x%02x)\n", tap_status);
            tap_type = 1;
        }
        if (int_source & ADXL345_INT_DOUBLE_TAP) {
            printk(KERN_INFO "adxl345: Double tap detected (status=0x%02x)\n", tap_status);
            tap_type = 2;
        }

        // sysfs: update tap_count, last_tap, complete tap_event if needed
        atomic_inc(&data->tap_count);
        mutex_lock(&data->tap_lock);
        data->last_tap = tap_type;
        if (data->tap_wait_busy)
            complete(&data->tap_event);
        mutex_unlock(&data->tap_lock);

        return IRQ_HANDLED;
    }
    return IRQ_NONE;
}

// Opérations supportées sur le fichier caractère
static const struct file_operations adxl345_fops = {
    .owner = THIS_MODULE,
    .read = adxl345_read,
    // Les autres opérations sont ignorées
};

// Fonction appelée lors de la détection du capteur (insmod)
static int adxl345_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    int ret = 0;
    u8 devid;
    struct adxl345_data *data = NULL;

    while (1) {
        // Vérifie l'identité du capteur
        ret = i2c_smbus_read_byte_data(client, ADXL345_DEVID_REG);

        if (ret < 0) break;

        devid = ret & 0xFF;

        if (devid != ADXL345_DEVID_VAL) {
            ret = -ENODEV;
            break;
        }

        // Configure la plage de mesure à +/-4g
        ret = i2c_smbus_write_byte_data(client, ADXL345_DATA_FORMAT_REG, ADXL345_DATA_FORMAT_4G);
        if (ret) break;

        // Met le capteur en mode mesure
        ret = i2c_smbus_write_byte_data(client, ADXL345_POWER_CTL_REG, ADXL345_POWER_CTL_MEASURE);
        if (ret) break;

        // Configure tap/double tap detection registers
        ret = i2c_smbus_write_byte_data(client, ADXL345_THRESH_TAP_REG, ADXL345_THRESH_TAP_VAL);
        if (ret) break;

        ret = i2c_smbus_write_byte_data(client, ADXL345_DUR_REG, ADXL345_DUR_VAL);
        if (ret) break;

        ret = i2c_smbus_write_byte_data(client, ADXL345_LATENT_REG, ADXL345_LATENT_VAL);
        if (ret) break;

        ret = i2c_smbus_write_byte_data(client, ADXL345_WINDOW_REG, ADXL345_WINDOW_VAL);
        if (ret) break;

        // Enable tap detection on Z axis
        ret = i2c_smbus_write_byte_data(client, ADXL345_TAP_AXES_REG, ADXL345_TAP_AXES_Z);
        if (ret) break;

        // Map all interrupts to INT1 (especially TAP/DOUBLE TAP)
        ret = i2c_smbus_write_byte_data(client, ADXL345_INT_MAP_REG, 0x00);
        if (ret) break;

        // Enable single and double tap interrupts
        ret = i2c_smbus_write_byte_data(client, ADXL345_INT_ENABLE_REG,
                                        ADXL345_INT_SINGLE_TAP | ADXL345_INT_DOUBLE_TAP);
        if (ret) break;

        // Alloue la structure de données du driver
        data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
        if (!data) {
            ret = -ENOMEM;
            break;
        }

        data->client = client;
        data->irq_requested = 0;
        i2c_set_clientdata(client, data);

        // sysfs: initialize tap state
        mutex_init(&data->tap_lock);
        data->tap_axis = 'z';
        data->tap_mode = 0;
        atomic_set(&data->tap_count, 0);
        init_completion(&data->tap_event);
        data->tap_wait_busy = 0;
        data->last_tap = 0;

        // Enregistre le périphérique caractère /dev/adxl345
        data->miscdev.minor = MISC_DYNAMIC_MINOR;
        data->miscdev.name = DEVICE_NAME;
        data->miscdev.fops = &adxl345_fops;
        data->miscdev.parent = &client->dev;
        
        ret = misc_register(&data->miscdev);
        if (ret) break;

        // Request threaded IRQ for tap/double tap
        if (client->irq > 0) {
            ret = request_threaded_irq(client->irq, NULL, adxl345_irq_thread,
                                       IRQF_ONESHOT | IRQF_TRIGGER_RISING,
                                       DEVICE_NAME, data);
            if (ret) {
                dev_warn(&client->dev, "adxl345: Failed to request IRQ %d\n", client->irq);
            } else {
                data->irq_requested = 1;
                dev_info(&client->dev, "adxl345: IRQ %d registered for tap detection\n", client->irq);
            }
        } else {
            dev_warn(&client->dev, "adxl345: No IRQ configured, tap detection disabled\n");
        }

        // Register sysfs attributes
        ret = device_create_file(&client->dev, &dev_attr_tap_axis);
        if (ret) break;
        ret = device_create_file(&client->dev, &dev_attr_tap_mode);
        if (ret) break;
        ret = device_create_file(&client->dev, &dev_attr_tap_wait);
        if (ret) break;
        ret = device_create_file(&client->dev, &dev_attr_tap_count);
        if (ret) break;

        dev_set_drvdata(&client->dev, data);

        dev_info(&client->dev, "adxl345: driver loaded\n");
        return 0;
    }

    // En cas d'erreur, met le capteur en veille
    i2c_smbus_write_byte_data(client, ADXL345_POWER_CTL_REG, ADXL345_POWER_CTL_STANDBY);
    return ret;
}

// Fonction appelée lors du retrait du driver (rmmod)
static void adxl345_remove(struct i2c_client *client) {
    struct adxl345_data *data = i2c_get_clientdata(client);

    // Remove sysfs attributes
    device_remove_file(&client->dev, &dev_attr_tap_axis);
    device_remove_file(&client->dev, &dev_attr_tap_mode);
    device_remove_file(&client->dev, &dev_attr_tap_wait);
    device_remove_file(&client->dev, &dev_attr_tap_count);

    // Disable tap interrupts
    i2c_smbus_write_byte_data(client, ADXL345_INT_ENABLE_REG, 0x00);

    // Met le capteur en veille et libère le device
    i2c_smbus_write_byte_data(client, ADXL345_POWER_CTL_REG, ADXL345_POWER_CTL_STANDBY);
    misc_deregister(&data->miscdev);

    // Free IRQ if requested
    if (data->irq_requested && client->irq > 0)
        free_irq(client->irq, data);

    dev_info(&client->dev, "adxl345: driver unloaded\n");
}

// Table d'identification I2C et OF
static const struct i2c_device_id adxl345_id[] = {{"adxl345", 0}, {}};
MODULE_DEVICE_TABLE(i2c, adxl345_id);

static const struct of_device_id adxl345_of_match[] = {{.compatible = "adi,adxl345"}, {}};
MODULE_DEVICE_TABLE(of, adxl345_of_match);

// Déclaration du driver I2C
static struct i2c_driver adxl345_driver = {
    .driver =
        {
            .name = "adxl345",
            .of_match_table = adxl345_of_match,
        },
    .probe = adxl345_probe,
    .remove = adxl345_remove,
    .id_table = adxl345_id,
};

module_i2c_driver(adxl345_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("REDS");
MODULE_DESCRIPTION("ADXL345 accelerometer simple char driver");