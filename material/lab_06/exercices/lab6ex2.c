#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>

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

// IRQ handler for tap/double tap
static irqreturn_t adxl345_irq_thread(int irq, void *dev_id) {
    struct adxl345_data *data = dev_id;
    struct i2c_client *client = data->client;
    int int_source, tap_status;

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
        }
        if (int_source & ADXL345_INT_DOUBLE_TAP) {
            printk(KERN_INFO "adxl345: Double tap detected (status=0x%02x)\n", tap_status);
        }
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