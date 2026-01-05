# Ultrasonic Distance Logger – Linux Device Driver

## Overview
A Device Tree based Linux kernel project that measures ultrasonic distance on button press, displays it on an I2C OLED, and logs the last 10 values persistently in an I2C EEPROM.

## Hardware
- Raspberry Pi 4B (BCM2711)
- HC-SR04 Ultrasonic Sensor
- Push Button (GPIO IRQ)
- I2C EEPROM (K24C256)
- I2C OLED (SH1106)

## Features
- GPIO interrupt driven button
- Deferred processing using workqueues
- Ultrasonic distance measurement using kernel timing APIs
- Circular EEPROM logging (last 10 values)
- OLED display update
- Character device `/dev/distance_logger`
- Device Tree based hardware configuration

## Project Structure
