obj-m = hello.o
obj-m += hello-1.o 
obj-m += hello-2.o 
obj-m += hello-3.o 
obj-m += hello-4.o 
obj-m += hello-5.o 
obj-m += startstop.o 
startstop-objs := start.o stop.o 
obj-m += char-device.o


KVERSION = $(shell uname -r)
PWD := $(CURDIR) 


all: 
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
fclean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions
