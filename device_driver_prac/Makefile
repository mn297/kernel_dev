obj-m += device_driver_dummy.o
obj-m += cdev.o
obj-m += dummy_platform_device.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
MODULENAME = dummy_platform_device cdev device_driver_dummy

all:
	make -C $(KERNELDIR) M=$(PWD) modules

clean:
	make -C $(KERNELDIR) M=$(PWD) clean

install: all
	sudo insmod device_driver_dummy.ko
	sudo insmod cdev.ko
	sudo insmod dummy_platform_device.ko

uninstall:
	sudo rmmod cdev
	sudo rmmod device_driver_dummy
	sudo rmmod dummy_platform_device

reinstall: uninstall install

.PHONY: all clean install uninstall reinstall
