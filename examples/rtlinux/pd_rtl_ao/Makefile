# Makefile.pd_rtl_ai

all: pd_rtl_ao.o 

# the path to the rt-linux kernel
RTLINUX_DIR=/usr/src/rtlinux
INCLUDE=-I$(RTLINUX_DIR)/linux/include -I$(RTLINUX_DIR)/include -I$(RTLINUX_DIR)/include/posix -I$(RTLINUX_DIR)/include/compat

PDROOT = ../../..

CFLAGS = -O2 -Wall ${INCLUDE} -I${PDROOT}/include -I/lib/modules/$(shell uname -r)/build/include -D__KERNEL__ -D__RT__ -DMODULE

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f pd_rtl_ao.o
