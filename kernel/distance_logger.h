#ifndef _DISTANCE_LOGGER_H_
#define _DISTANCE_LOGGER_H_

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/wait.h>

#define DEVICE_NAME   "distance_logger"
#define CLASS_NAME    "distance"
#define MAX_LOG_ENTRIES 10

struct distance_logger_dev {
	dev_t devt;
	struct cdev cdev;
	struct class *class;
	struct device *device;

	/* state */
	u32 last_distance_cm;
	bool logging_enabled;

	/* sync (future use) */
	struct mutex lock;
	wait_queue_head_t read_queue;
};

/* button_driver.c */
int button_driver_init(struct distance_logger_dev *dl_dev);
void button_driver_exit(void);

/* ultrasonic_driver.c */
int ultrasonic_driver_init(struct distance_logger_dev *dl_dev);
void ultrasonic_driver_exit(void);

/* eeprom_driver.c */
int eeprom_driver_init(struct distance_logger_dev *dl_dev);
void eeprom_driver_exit(void);

/* oled_driver.c */
int oled_driver_init(struct distance_logger_dev *dl_dev);
void oled_driver_exit(void);

/* ultrasonic -> eeprom + oled */
int ultrasonic_measure_and_process(void);

/* ultrasonic -> oled */
int oled_display_distance(u32 distance);

/* ultrasonic -> eeprom */
void eeprom_log_store(u16 distance);

#endif 
