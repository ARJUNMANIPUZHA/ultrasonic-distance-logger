#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>

#include "distance_logger.h"


/* ultrasonic driver api */
extern int ultrasonic_measure_and_process(void);

struct button_dev {
	struct device       *dev;
	struct gpio_desc    *button;
	int                  irq;
	struct work_struct   work;
};

/* Workqueue */

static void button_work_fn(struct work_struct *work)
{
	struct button_dev *bdev = container_of(work, struct button_dev, work);

	dev_info(bdev->dev, "Button pressed, triggering measurement\n");

	ultrasonic_measure_and_process();
}

/* IRQ handler */

static irqreturn_t button_irq_handler(int irq, void *data)
{
	struct button_dev *bdev = data;

	/* defer work */
	schedule_work(&bdev->work);

	return IRQ_HANDLED;
}

/* probe */

static int button_probe(struct platform_device *pdev)
{
	struct button_dev *bdev;
	int ret;

	bdev = devm_kzalloc(&pdev->dev, sizeof(*bdev), GFP_KERNEL);
	if (!bdev)
		return -ENOMEM;

	bdev->dev = &pdev->dev;

	INIT_WORK(&bdev->work, button_work_fn);

	/* get button gpio from DT */
	bdev->button = devm_gpiod_get(&pdev->dev, "button", GPIOD_IN);
	if (IS_ERR(bdev->button))
		return PTR_ERR(bdev->button);

	bdev->irq = gpiod_to_irq(bdev->button);
	if (bdev->irq < 0)
		return bdev->irq;

	ret = devm_request_irq(&pdev->dev, bdev->irq, button_irq_handler, IRQF_TRIGGER_FALLING, "distance_button", bdev);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, bdev);

	dev_info(&pdev->dev, "Button IRQ driver initialized\n");
	return 0;
}

/* remove */

static int button_remove(struct platform_device *pdev)
{
	struct button_dev *bdev = platform_get_drvdata(pdev);

	cancel_work_sync(&bdev->work);
	return 0;
}

/* DT match */

static const struct of_device_id button_of_match[] = {
	{ .compatible = "custom,distance-button" },
	{ }
};
MODULE_DEVICE_TABLE(of, button_of_match);

static struct platform_driver button_driver = {
	.probe  = button_probe,
	.remove = button_remove,
	.driver = {
		.name = "distance_button",
		.of_match_table = button_of_match,
	},
};

int button_driver_init(struct distance_logger_dev *dl_dev)
{
	return platform_driver_register(&button_driver);
}

void button_driver_exit(void)
{
	platform_driver_unregister(&button_driver);
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arjun Krishna");
MODULE_DESCRIPTION("External Push Button IRQ Driver (Distance Logger)");
