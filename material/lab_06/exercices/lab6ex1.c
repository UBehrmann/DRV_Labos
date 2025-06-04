#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>

#define ADXL345_DEVID_REG 0x00
#define ADXL345_DEVID_VAL 0xE5
#define ADXL345_DATA_FORMAT_REG 0x31
#define ADXL345_DATA_FORMAT_4G 0x01  // +/-4g, autres bits à 0
#define ADXL345_POWER_CTL_REG 0x2D
#define ADXL345_POWER_CTL_MEASURE 0x08
#define ADXL345_POWER_CTL_STANDBY 0x00
#define ADXL345_DATAX0_REG 0x32
#define ADXL345_DATA_LEN 6  // 2 octets par axe (X, Y, Z)

#define DEVICE_NAME "adxl345"

// ADXL345 register map for reference
// 0x00 - DEVID
// 0x2D - POWER_CTL
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
    char outbuf[64];
    char xbuf[16], ybuf[16], zbuf[16];
    int len, ret;

    if (*offset > 0)
        return 0;

    ret = adxl345_read_axes(data->client, &x, &y, &z);
    if (ret)
        return ret;

    adxl345_to_g(x, xbuf);
    adxl345_to_g(y, ybuf);
    adxl345_to_g(z, zbuf);

    len = snprintf(outbuf, sizeof(outbuf), "X = %s; Y = %s; Z = %s\n", xbuf, ybuf, zbuf);

    if (len > count)
        len = count;

    if (copy_to_user(buf, outbuf, len))
        return -EFAULT;

    *offset += len;
    return len;
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

        if (ret < 0)
            break;

        devid = ret & 0xFF;

        if (devid != ADXL345_DEVID_VAL) {
            ret = -ENODEV;
            break;
        }

        // Configure la plage de mesure à +/-4g
        ret = i2c_smbus_write_byte_data(client, ADXL345_DATA_FORMAT_REG, ADXL345_DATA_FORMAT_4G);
        if (ret)
            break;

        // Met le capteur en mode mesure
        ret = i2c_smbus_write_byte_data(client, ADXL345_POWER_CTL_REG, ADXL345_POWER_CTL_MEASURE);
        if (ret)
            break;

        // Alloue la structure de données du driver
        data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
        if (!data) {
            ret = -ENOMEM;
            break;
        }

        data->client = client;
        i2c_set_clientdata(client, data);

        // Enregistre le périphérique caractère /dev/adxl345
        data->miscdev.minor = MISC_DYNAMIC_MINOR;
        data->miscdev.name = DEVICE_NAME;
        data->miscdev.fops = &adxl345_fops;
        data->miscdev.parent = &client->dev;
        
        ret = misc_register(&data->miscdev);
        if (ret)
            break;

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
    // Met le capteur en veille et libère le device
    i2c_smbus_write_byte_data(client, ADXL345_POWER_CTL_REG, ADXL345_POWER_CTL_STANDBY);
    misc_deregister(&data->miscdev);
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
