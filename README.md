# Ultrasonic Distance Logger – Linux Device Driver
https://youtu.be/JToF8V5raUw?si=COt8OYYojzjY4xW4

# Ultrasonic Distance Logger

## Overview

This project implements a Linux kernel driver–based distance logging system on Raspberry Pi.
An external push button triggers an ultrasonic distance measurement. 
The measured distance is:
  * displayed on an I2C OLED
  * stored persistently in an I2C EEPROM, (Only the last 10 values are stored using a circular buffer. The data remains available even after reboot)

## Key Features
  * Button-triggered ultrasonic measurement (no polling)
  * GPIO interrupt with deferred processing using workqueue
  * Distance display on I2C OLED
  * Persistent EEPROM logging (circular buffer)
  * Character device `/dev/distance_logger`
  * Device Tree based hardware configuration

## Hardware Use
  * Raspberry Pi (BCM2711)
  * HC-SR04 Ultrasonic Sensor
  * Push Button (active-low)
  * I2C EEPROM (0x50)
  * I2C OLED (0x3C)
  * 
## How It Works
  1. Button press triggers a GPIO interrupt
  2. IRQ handler schedules work using `schedule_work()`
  3. Ultrasonic distance is measured in process context
  4. Distance is displayed on OLED
  5. Distance is stored in EEPROM

## Project Structure
├── dts
│   └── distance_logger_overlay.dts
├── kernel
│   ├── button_driver.c
│   ├── distance_logger.h
│   ├── eeprom_driver.c
│   ├── logger_ch_driver.c
│   ├── oled_driver.c
│   └── ultrasonic_driver.c
├── Makefile
└── README.md

## EEPROM Log
Read stored values using:

```
cat /sys/bus/i2c/devices/1-0050/log
```
## Build & Load
```
make
sudo insmod distance_logger.ko
```
## Kernel Concepts Used

* Character device drivers
* GPIO interrupts
* Workqueues
* Device Tree
* I2C drivers
* Kernel timing and synchronization

Demo : 
https://youtu.be/JToF8V5raUw?si=COt8OYYojzjY4xW4


Just tell me 👍

