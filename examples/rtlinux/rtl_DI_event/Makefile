# Makefile.pd_rtl_ai

OBJECT = rtl_DI_event.o

all: $(OBJECT) 

# set this variable to the RT Linux headers location
RTLINUX_DIR=/usr/src/rtlinux
INCLUDE=-I$(RTLINUX_DIR)/linux/include -I$(RTLINUX_DIR)/include -I$(RTLINUX_DIR)/include/posix -I$(RTLINUX_DIR)/include/compat

PDROOT = ../../..

CFLAGS = -O2 -Wall ${INCLUDE} -I${PDROOT}/include -I/lib/modules/$(shell uname -r)/build/include -D__KERNEL__ -D__RT__ -DMODULE

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<


clean:
	rm -f $(OBJECT)
