#XP kernel pathus/local/arm/arm-2009q3/bin/arm-none-linux-gnueabi-gcc
4412_KERNEL_DIR = /root/android_4.0.4_dma4412L/kernel_dma4412L
##/home/joeko/android-kernel-dma4412u
ifneq ($(KERNELRELEASE),)
	obj-m := tm1637.o
else
	KERNELDIR ?= $(4412_KERNEL_DIR)
	
	PWD := $(shell pwd)
	
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	
clean:
	rm -f *.ko *.o *.bak *.mod.*
endif 


