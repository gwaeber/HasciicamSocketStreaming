obj-m += io_dd.o

ODROID_LINUX_DIR=/home/gilles/workspace/xu3/buildroot/output/build/linux-4.6.4/

KDIR    := $(ODROID_LINUX_DIR)
PWD	:= $(shell pwd)

default:
	make -C $(KDIR) M=$(PWD) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- modules

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f modules.order
	rm -rf .tmp_versions/
	rm -f Module.markers

