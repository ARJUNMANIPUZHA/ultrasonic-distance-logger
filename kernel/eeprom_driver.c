#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/sysfs.h>

#include "distance_logger.h"


#define EEPROM_META_ADDR    0x0000
#define EEPROM_DATA_START   0x0010
#define MAX_LOG_ENTRIES     10

struct eeprom_dev {
	struct i2c_client *client;
	u8 write_idx;
	u8 valid_count;
	struct mutex lock;
};

static struct eeprom_dev *global_eeprom;

/* ---------- low-level helpers ---------- */

static int eeprom_write(struct i2c_client *client, u16 addr, u8 *buf, u8 len)
{
	u8 tmp[4];

	tmp[0] = addr >> 8;
	tmp[1] = addr & 0xff;
	memcpy(&tmp[2], buf, len);

	if (i2c_master_send(client, tmp, len + 2) != len + 2)
		return -EIO;

	msleep(10);
	return 0;
}

static int eeprom_read(struct i2c_client *client, u16 addr, u8 *buf, u8 len)
{
	u8 a[2] = { addr >> 8, addr & 0xff };

	if (i2c_master_send(client, a, 2) != 2)
		return -EIO;

	if (i2c_master_recv(client, buf, len) != len)
		return -EIO;

	return 0;
}

void eeprom_log_store(u16 distance)
{
	u16 addr;
	u8 data[2];
	u8 meta;

	if (!global_eeprom)
		return;

	mutex_lock(&global_eeprom->lock);

	addr = EEPROM_DATA_START + (global_eeprom->write_idx * 2);

	data[0] = distance >> 8;
	data[1] = distance & 0xff;

	eeprom_write(global_eeprom->client, addr, data, 2);

	global_eeprom->write_idx = (global_eeprom->write_idx + 1) % MAX_LOG_ENTRIES;

	if (global_eeprom->valid_count < MAX_LOG_ENTRIES)
		global_eeprom->valid_count++;

	meta = global_eeprom->write_idx;
	eeprom_write(global_eeprom->client, EEPROM_META_ADDR, &meta, 1);

	mutex_unlock(&global_eeprom->lock);
}

EXPORT_SYMBOL_GPL(eeprom_log_store);

/* sysfs */

static ssize_t log_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct eeprom_dev *edev = dev_get_drvdata(dev);
	u8 idx;
	u16 val;
	u8 data[2];
	int i, len = 0;

	mutex_lock(&edev->lock);

	idx = edev->write_idx;

	for (i = 0; i < edev->valid_count; i++) 
	{
		u16 addr = EEPROM_DATA_START + (idx * 2);

		if (!eeprom_read(edev->client, addr, data, 2)) 
		{
			val = (data[0] << 8) | data[1];
			len += scnprintf(buf + len, PAGE_SIZE - len, "%u\n", val);
		}

		idx = (idx + 1) % MAX_LOG_ENTRIES;
	}

	mutex_unlock(&edev->lock);
	return len;
}

static DEVICE_ATTR_RO(log);

static int eeprom_probe(struct i2c_client *client)
{
	struct eeprom_dev *edev;
	u8 meta = 0;

	edev = devm_kzalloc(&client->dev, sizeof(*edev), GFP_KERNEL);
	if (!edev)
		return -ENOMEM;

	mutex_init(&edev->lock);
	edev->client = client;

	eeprom_read(client, EEPROM_META_ADDR, &meta, 1);
	edev->write_idx = meta % MAX_LOG_ENTRIES;
	edev->valid_count = 0;

	i2c_set_clientdata(client, edev);
	global_eeprom = edev;

	device_create_file(&client->dev, &dev_attr_log);

	dev_info(&client->dev, "EEPROM logger ready\n");
	return 0;
}

static void eeprom_remove(struct i2c_client *client)
{
	device_remove_file(&client->dev, &dev_attr_log);
	global_eeprom = NULL;
}

static const struct of_device_id eeprom_of_match[] = {
	{ .compatible = "custom,eeprom-logger" },
	{ }
};
MODULE_DEVICE_TABLE(of, eeprom_of_match);

static struct i2c_driver eeprom_driver = {
	.probe_new = eeprom_probe,
	.remove = eeprom_remove,
	.driver = {
		.name = "distance_eeprom",
		.of_match_table = eeprom_of_match,
	},
};

int eeprom_driver_init(struct distance_logger_dev *dl_dev)
{
	return i2c_add_driver(&eeprom_driver);
}

void eeprom_driver_exit(void)
{
	i2c_del_driver(&eeprom_driver);
}



MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arjun Krishna");
MODULE_DESCRIPTION("I2C EEPROM Circular Distance Logger");
