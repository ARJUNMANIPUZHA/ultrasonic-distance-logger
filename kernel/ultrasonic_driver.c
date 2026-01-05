#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/completion.h>

#include "distance_logger.h"


/* api from eeprom and oled */
extern void eeprom_log_store(u16 distance);
extern int oled_display_distance(u32 distance);

#define MAX_DISTANCE_CM   400

struct ultrasonic_dev {
	struct device     *dev;
	struct gpio_desc  *trig;
	struct gpio_desc  *echo;
	int                echo_irq;

	ktime_t            rise_time;
	u32                distance_cm;

	struct completion  echo_done;
	struct mutex       lock;
};

static struct ultrasonic_dev *global_udev;

/* irq handler */
static irqreturn_t echo_irq_handler(int irq, void *data)
{
	struct ultrasonic_dev *udev = data;
	ktime_t now = ktime_get();

	if (gpiod_get_value(udev->echo)) 
	{
		/* rising edge */
		udev->rise_time = now;
	} 
	else 
	{
		/* falling edge */
		s64 delta_ns = ktime_to_ns(ktime_sub(now, udev->rise_time));

		mutex_lock(&udev->lock);
		udev->distance_cm = (u32)(delta_ns / 58000);
		mutex_unlock(&udev->lock);

		complete(&udev->echo_done);
	}
	return IRQ_HANDLED;
}

static int ultrasonic_trigger(struct ultrasonic_dev *udev)
{
	reinit_completion(&udev->echo_done);

	/* trigger pulse */
	gpiod_set_value(udev->trig, 1);
	usleep_range(10, 15);
	gpiod_set_value(udev->trig, 0);

	/* wait for echo */
	if (!wait_for_completion_timeout(&udev->echo_done, msecs_to_jiffies(30))) 
	{
		dev_warn(udev->dev, "Ultrasonic timeout\n");
		return -ETIMEDOUT;
	}
	return 0;
}

/* this api will be called from button driver */
int ultrasonic_measure_and_process(void)
{
	u32 distance;
	int ret;

	if (!global_udev)
		return -ENODEV;

	ret = ultrasonic_trigger(global_udev);
	if (ret)
		return ret;

	mutex_lock(&global_udev->lock);
	distance = global_udev->distance_cm;
	mutex_unlock(&global_udev->lock);

	if (distance == 0 || distance > MAX_DISTANCE_CM) 
	{
		dev_warn(global_udev->dev, "Invalid distance: %u\n", distance);
		return -EINVAL;
	}

	/* store and display */
	eeprom_log_store((u16)distance);
	oled_display_distance(distance);

	dev_info(global_udev->dev, "Distance measured: %u cm\n", distance);

	return 0;
}

EXPORT_SYMBOL_GPL(ultrasonic_measure_and_process);

static int ultrasonic_probe(struct platform_device *pdev)
{
	struct ultrasonic_dev *udev;
	int ret;

	udev = devm_kzalloc(&pdev->dev, sizeof(*udev), GFP_KERNEL);
	if (!udev)
		return -ENOMEM;

	udev->dev = &pdev->dev;
	mutex_init(&udev->lock);
	init_completion(&udev->echo_done);

	udev->trig = devm_gpiod_get(&pdev->dev, "trig", GPIOD_OUT_LOW);
	if (IS_ERR(udev->trig))
		return PTR_ERR(udev->trig);

	udev->echo = devm_gpiod_get(&pdev->dev, "echo", GPIOD_IN);
	if (IS_ERR(udev->echo))
		return PTR_ERR(udev->echo);

	udev->echo_irq = gpiod_to_irq(udev->echo);
	if (udev->echo_irq < 0)
		return udev->echo_irq;

	ret = devm_request_irq(&pdev->dev, udev->echo_irq, echo_irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "ultrasonic_echo",udev);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, udev);
	global_udev = udev;

	dev_info(&pdev->dev, "Ultrasonic driver ready\n");
	return 0;
}

static int ultrasonic_remove(struct platform_device *pdev)
{
	global_udev = NULL;
	return 0;
}

/* DT match */
static const struct of_device_id ultrasonic_of_match[] = {
	{ .compatible = "custom,hcsr04" },
	{ }
};
MODULE_DEVICE_TABLE(of, ultrasonic_of_match);

static struct platform_driver ultrasonic_driver = {
	.probe  = ultrasonic_probe,
	.remove = ultrasonic_remove,
	.driver = {
		.name = "distance_ultrasonic",
		.of_match_table = ultrasonic_of_match,
	},
};

static struct distance_logger_dev *g_dl_dev;

int ultrasonic_driver_init(struct distance_logger_dev *dl_dev)
{
	g_dl_dev = dl_dev;
	return platform_driver_register(&ultrasonic_driver);
}

void ultrasonic_driver_exit(void)
{
	platform_driver_unregister(&ultrasonic_driver);
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arjun Krishna");
MODULE_DESCRIPTION("Ultrasonic HC-SR04 Driver");
