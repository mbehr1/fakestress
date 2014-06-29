# if KERNELRELEASE is defined it's invoked from the
# kernel build system
ifneq ($(KERNELRELEASE),)

obj-m := fakestress.o
#otherwise we're called e.g. from the command
# line; invoke the kernel build system.
else
ifeq ($(BUILD_KERNEL_VERSION),)
	BUILD_KERNEL_VERSION := $(shell uname -r)
endif
	KERNELDIR ?= /lib/modules/$(BUILD_KERNEL_VERSION)/build
	PWD := $(shell pwd)
	
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
		
endif
