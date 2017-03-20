obj-m := scull.o

KERNELDIR := /lib/modules/3.2.79/build
PWD := $(shell pwd)


modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install


clean:
	rm -rf *.o *.ko *.mod.c *.order *.symvers
