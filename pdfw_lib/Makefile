
CC=gcc
DEBUG=0
RTL=0
RTAI=0

ifeq ($(KERNEL_26),1)

ifneq ($(KERNELRELEASE),)

pwrdaq-objs += ppdfw_lib.o

else

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
endif

else


all:  pdfw_lib.o
#	clear
pdfw_lib.o: pdfw_lib.c pdl_ain.c pdl_ao.c pdl_dio.c pdl_fwi.c pdl_brd.c pdl_event.c pdl_aio.c pdl_init.c pdl_int.c

# we reuse the CFLAGS variable exported by the parent Makefile
.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<
	
endif
	
install:

clean:
	rm -f *.o *.ko *.cmd
	

