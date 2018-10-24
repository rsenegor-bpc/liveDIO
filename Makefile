
CC=gcc
LD=ld
export VERSION_MAJOR=3
export VERSION_MINOR=6
export VERSION_EXTRA=26
SUBARCH=$(shell uname -m)

DEBUGFLAGS=-DPD_DEBUG -g

RTLINUXPRO_DIR=/opt/rtldk-2.2
RTLINUX_DIR=/usr/src/rtlinux
RTAI_DIR=/usr/src/rtai
XENOMAI_DIR=/usr/xenomai
RTLFLAGS=-D_PD_RTL -I$(RTLINUX_DIR)/linux/include -I$(RTLINUX_DIR)/include -I$(RTLINUX_DIR)/include/posix -I$(RTLINUX_DIR)/include/compat
RTLPROFLAGS=-nostdinc -D_PD_RTLPRO -D__RTCORE_KERNEL__ -isystem /lib/modules/$(shell uname -r)/build/include -I$(RTLINUXPRO_DIR)/rtlinuxpro/include -I$(RTLINUXPRO_DIR)/rtlinuxpro/include/app -I$(RTLINUXPRO_DIR)/rtlinuxpro/include/app/rtl
RTAIFLAGS=-D_PD_RTAI -I$(RTAI_DIR)/include
XENOMAIFLAGS=-D_PD_XENOMAI -I$(XENOMAI_DIR)/include

PDSUBDIRS=lib 

# Parse kernel version, the method to compile a kernel module
# changed between 2.4 and 2.6 or 3.0 & up
ifeq ($(shell uname -r | awk -F"." '{print $$1.$$2}'),26)
    NEW_KERNEL=1
else
    ifeq ($(shell uname -r | awk -F"." '{print $$1}'),2)
        NEW_KERNEL=0
    else
        NEW_KERNEL=1
    endif
endif


ifeq ($(NEW_KERNEL),1)

ifeq ($(RTLPRO), 1)
include $(RTLINUXPRO_DIR)/rtlinuxpro/rtl.mk

CFLAGS := $(filter-out -msoft-float,$(CFLAGS)) -mhard-float
CFLAGS += -D_PD_RTLPRO -DPD_VERSION_MAJOR=$(VERSION_MAJOR) -DPD_VERSION_MINOR=$(VERSION_MINOR) -DPD_VERSION_EXTRA=$(VERSION_EXTRA)

ifeq ($(DEBUG), 1)
CFLAGS += $(DEBUGFLAGS)
endif

all: pwrdaq.o pwrdaq.rtl

pwrdaq.o : powerdaq.o \
           powerdaq_isr.o \
	   powerdaq_osal.o \
	   pdfw_lib/pdfw_lib.o \
	   lib/powerdaq32.o \
	   lib/pd_hcaps.o
	$(LD) -r -o pwrdaq.o $?

include $(RTLINUXPRO_DIR)/rtlinuxpro/Rules.make
else

PDDRIVER=pwrdaq.ko

ifneq ($(KERNELRELEASE)$(KERNELVERSION),)

EXTRA_CFLAGS += -DPD_VERSION_MAJOR=$(VERSION_MAJOR) -DPD_VERSION_MINOR=$(VERSION_MINOR) -DPD_VERSION_EXTRA=$(VERSION_EXTRA)

ifeq ($(DEBUG),1)
	EXTRA_CFLAGS += ${DEBUGFLAGS}
endif
ifeq ($(RTL),1)
	EXTRA_CFLAGS += $(RTLFLAGS)
endif
ifeq ($(RTAI),1)
	EXTRA_CFLAGS += -D_PD_RTAI $(shell $(RTAI_DIR)/bin/rtai-config --module-cflags) -I/usr/include
endif
ifeq ($(XENOMAI),1)
	EXTRA_CFLAGS += -D_PD_XENOMAI $(shell $(XENOMAI_DIR)/bin/xeno-config --xeno-cflags)
endif
ifeq ($(SUBARCH),x86_64)
	EXTRA_CFLAGS += -mcmodel=kernel -mno-red-zone
endif

obj-m := pwrdaq.o 
#obj-m += pdfw_lib/
pwrdaq-objs := powerdaq.o powerdaq_isr.o powerdaq_osal.o pdfw_lib/pdfw_lib.o 

else

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
CFLAGS += -I/usr/include/asm

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
	mv $(PDDRIVER) ./drv
	for i in $(PDSUBDIRS); do $(MAKE) -C $$i VERSION_MAJOR=$(VERSION_MAJOR) VERSION_MINOR=$(VERSION_MINOR) VERSION_EXTRA=$(VERSION_EXTRA); done; 
endif

endif

else

PDDRIVER=pwrdaq.o

export CFLAGS=-DEXPORT_SYMTAB -D__KERNEL__ -DMODULE -O2 -Wall -I/lib/modules/$(shell uname -r)/build/include -fno-strict-aliasing

CFLAGS += -DPD_VERSION_MAJOR=$(VERSION_MAJOR) -DPD_VERSION_MINOR=$(VERSION_MINOR) -DPD_VERSION_EXTRA=$(VERSION_EXTRA)

ifeq ($(DEBUG),1)
	CFLAGS += ${DEBUGFLAGS}
endif
ifeq ($(RTL),1)
	CFLAGS += $(RTLFLAGS)
endif
ifeq ($(RTLPRO),1)
	CFLAGS += $(RTLPROFLAGS)
endif
ifeq ($(RTAI),1)
	CFLAGS += $(RTAIFLAGS)
endif
ifeq ($(SUBARCH),x86_64)
	CFLAGS += -mcmodel=kernel -mno-red-zone
endif

ifeq ($(shell uname -r | awk -F"." '{print $$1.$$2}'),24)
# extra test for Red Hat 2.4.20 kernel
ifeq ($(shell grep 'remap_page_range([^,]*vm_area_struct.*)' /lib/modules/`uname -r`/build/include/linux/mm.h | wc -l | tr -cd 0-9),1)
	CFLAGS += -DPD_USING_REDHAT_2420
endif
endif

ifeq ($(shell uname -r | awk -F"." '{print $$1.$$2}'),22)
# extra tests for Red Hat 2.2 kernels
ifeq ($(shell grep 'kill_fasync(.*,.*,.*)' /usr/include/linux/fs.h | wc -l | tr -cd 0-9),1)
	CFLAGS += -DPD_USING_REDHAT_62
endif
ifeq ($(shell grep 'typedef.*wait_queue_head_t;' /usr/include/linux/wait.h | wc -l | tr -cd 0-9),1)
	CFLAGS += -DCOMPAT_V22
endif
endif


all:  $(PDSUBDIRS) $(PDDRIVER)

.PHONY: subdirs $(PDSUBDIRS)

subdirs: $(PDSUBDIRS)

$(PDSUBDIRS):
	cd $@ && $(MAKE)
	
$(PDDRIVER): powerdaq.o powerdaq_isr.o powerdaq_osal.o pdfw_lib/pdfw_lib.o
	$(LD) -r $^ -o drv/$@
	
ifeq ($(shell uname -r | awk -F"." '{print $$1.$$2}'),22)
powerdaq_osal.o powerdaq_isr.o:
	$(CC) -c $(CFLAGS) -D__NO_VERSION__ -o $@ `basename $@ .o`.c
endif

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<
	
endif
	
install:
	@(if [ `id -u` != "0" ]; then \
	   echo "You must be root to install the PowerDAQ driver"; \
	   exit -1;  \
	fi)
	@(if ! grep PDROOT /etc/profile > /dev/null; then \
	   echo "" >> /etc/profile; \
	   echo "#PowerDAQ setup: This line was added by the PowerDAQ driver install script" >> /etc/profile; \
	   echo "PDROOT=$(shell pwd)" >> /etc/profile; \
	   echo "export PDROOT" >> /etc/profile; \
	   echo "#PowerDAQ setup end" >> /etc/profile; \
	fi)
	for i in $(PDSUBDIRS); do $(MAKE) install -C $$i VERSION_MAJOR=$(VERSION_MAJOR) VERSION_MINOR=$(VERSION_MINOR) VERSION_EXTRA=$(VERSION_EXTRA); done;
	install -d /lib/modules/$(shell uname -r)/kernel/drivers/misc
	install -c ./drv/$(PDDRIVER) /lib/modules/$(shell uname -r)/kernel/drivers/misc
	/sbin/depmod -a
	./make-devices
	/sbin/ldconfig
	@echo "PowerDAQ driver for Linux is now installed"
	@echo "You can load the driver with the command 'modprobe pwrdaq'"
	@echo "You can compile the examples with the command 'make test'"
	
test:
	cd examples && $(MAKE)

clean:
	for i in $(PDSUBDIRS); do $(MAKE) clean -C $$i; done; 
	$(MAKE) clean -C examples
	$(MAKE) clean -C pdfw_lib
	rm -f *.o *.ko .*.cmd
	rm -f drv/*

