# set this variable to the RTAI Linux headers location
RTAI_DIR := /usr/rtai
PDROOT := $(shell pwd)/../../..
INCLUDE := -I$(RTAI_DIR)/include -I$(PDROOT)/include

ifeq ($(shell uname -r | awk -F"." '{print $$2}'),6)

ifneq ($(KERNELRELEASE),)
EXTRA_CFLAGS += $(shell $(RTAI_DIR)/bin/rtai-config --module-cflags) -I/usr/include -I$(PWD)/../../../include
	
obj-m := rtai_DIO.o


else

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
CFLAGS += -I/usr/include/asm 

all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

endif

else
all: rtai_DIO.o

CFLAGS = -O2 -Wall ${INCLUDE} -I/lib/modules/$(shell uname -r)/build/include -D__KERNEL__ -D__RT__ -DMODULE

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<
endif

clean:
	rm -f *.o *.ko
