obj-m += led_driver.o

KDIR = /home/chnam24/buildroot-2023.11.1/output/build/linux-custom
CROSS = /home/chnam24/buildroot-2023.11.1/output/host/bin/arm-buildroot-linux-gnueabihf-

all:
	make ARCH=arm CROSS_COMPILE=$(CROSS) -C $(KDIR) M=$(PWD) modules

clean:
	make ARCH=arm CROSS_COMPILE=$(CROSS) -C $(KDIR) M=$(PWD) clean
