# Context dependent makefile that can be called directly and will invoke itself
# through the kernel module building system.
KERNELDIR=/usr/src/linux


ifneq ($(KERNELRELEASE),)

EXTRA_CFLAGS+=-I $(PWD) -ARCH=um
secvault-objs := main.o sv_data.o

obj-m   := secvault.o


else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)

all:	svctl modules
svctl:	svctl.c
	gcc svctl.c util.c -o svctl
modules:
	$(MAKE) V=1 ARCH=um -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm -rf Module.symvers .*.cmd *.ko .*.o *.o *.mod.c .tmp_versions *.order svctl

endif
