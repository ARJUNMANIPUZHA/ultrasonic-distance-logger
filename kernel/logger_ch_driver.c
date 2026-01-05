#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#include "distance_logger.h"   /* shared header */

/* Global variables */
static dev_t logger_devt;
static struct cdev logger_cdev;
static struct class *logger_class;
static struct distance_logger_dev dl_dev;

/* file operations */
static int logger_open(struct inode *inode, struct file *file)
{
	pr_info("distance_logger: device opened\n");
	return 0;
}

static int logger_release(struct inode *inode, struct file *file)
{
	pr_info("distance_logger: device closed\n");
	return 0;
}

static ssize_t logger_read(struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
	char kbuf[32];
	int ret;

	/* simple one-shot read */
	if (*ppos > 0)
		return 0;

	ret = snprintf(kbuf, sizeof(kbuf), "Use button to log distance\n");

	if (copy_to_user(buf, kbuf, ret))
		return -EFAULT;

	*ppos += ret;
	return ret;
}

static const struct file_operations logger_fops = {
	.owner   = THIS_MODULE,
	.open    = logger_open,
	.release = logger_release,
	.read    = logger_read,
};

/* module init */
static int __init distance_logger_init(void)
{
	int ret;
	
	ret = alloc_chrdev_region(&logger_devt, 0, 1, "distance_logger");
	if (ret)
		return ret;

	cdev_init(&logger_cdev, &logger_fops);
	ret = cdev_add(&logger_cdev, logger_devt, 1);
	if (ret)
		goto err_cdev;

	logger_class = class_create(THIS_MODULE, "distance");
	if (IS_ERR(logger_class)) 
	{
		ret = PTR_ERR(logger_class);
		goto err_class;
	}

	device_create(logger_class, NULL, logger_devt, NULL, "distance_logger");

	ret = ultrasonic_driver_init(&dl_dev);
	if (ret)
		goto err_ultra;

	ret = eeprom_driver_init(&dl_dev);
	if (ret)
		goto err_eeprom;

	ret = oled_driver_init(&dl_dev);
	if (ret)
		goto err_oled;

	ret = button_driver_init(&dl_dev);
	if (ret)
		goto err_button;

	pr_info("distance_logger: module loaded\n");
	return 0;

err_button:
	oled_driver_exit();
err_oled:
	eeprom_driver_exit();
err_eeprom:
	ultrasonic_driver_exit();
err_ultra:
	device_destroy(logger_class, logger_devt);
	class_destroy(logger_class);
err_class:
	cdev_del(&logger_cdev);
err_cdev:
	unregister_chrdev_region(logger_devt, 1);
	return ret;
}

static void __exit distance_logger_exit(void)
{
	button_driver_exit();
	oled_driver_exit();
	eeprom_driver_exit();
	ultrasonic_driver_exit();

	device_destroy(logger_class, logger_devt);
	class_destroy(logger_class);

	cdev_del(&logger_cdev);
	unregister_chrdev_region(logger_devt, 1);

	pr_info("distance_logger: module unloaded\n");
}

module_init(distance_logger_init);
module_exit(distance_logger_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arjun Krishna");
MODULE_DESCRIPTION("Ultrasonic Distance Logger (Char Driver)");
