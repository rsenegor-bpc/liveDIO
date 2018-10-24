# Makefile.rtl_dma_ai


OBJECT =  rtbm_ai.o

all: $(OBJECT)  

# the path to the rt-linux kernel
RTLINUX_DIR=/usr/src/rtlinux
INCLUDE=-I$(RTLINUX_DIR)/linux/include -I$(RTLINUX_DIR)/include -I$(RTLINUX_DIR)/include/posix -I$(RTLINUX_DIR)/include/compat

PDROOT = ../../..

CFLAGS = -O2 -Wall ${INCLUDE} -I${PDROOT}/include -I/lib/modules/$(shell uname -r)/build/include -D__KERNEL__ -D__RT__ -DMODULE -DEXPORT_SYMTAB

rtbm_ai.o: rtl_rtbm_ai.o uct.o
	$(LD) -r $^ -o $@

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f $(OBJECT)
