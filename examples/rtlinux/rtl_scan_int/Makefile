# Makefile.rtl_int_ai

all: scan_int.o 

# the path to the rt-linux kernel
RTLINUX_DIR=/usr/src/rtlinux
INCLUDE=-I$(RTLINUX_DIR)/linux/include -I$(RTLINUX_DIR)/include -I$(RTLINUX_DIR)/include/posix -I$(RTLINUX_DIR)/include/compat

PDROOT = ../../..

CFLAGS = -O2 -Wall ${INCLUDE} -I${PDROOT}/include -I/lib/modules/$(shell uname -r)/build/include -D__KERNEL__ -D__RT__ -DMODULE

scan_int.o: rtl_scan_int.o scan_uct.o
	$(LD) -r $^ -o $@

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f *.o
