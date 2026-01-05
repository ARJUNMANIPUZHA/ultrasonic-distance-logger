# Build one main kernel module
obj-m += distance_logger.o

# Object files that form the module
distance_logger-objs := \
	kernel/logger_ch_driver.o \
	kernel/button_driver.o \
	kernel/ultrasonic_driver.o \
	kernel/eeprom_driver.o \
	kernel/oled_driver.o

# Kernel build directory
KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
