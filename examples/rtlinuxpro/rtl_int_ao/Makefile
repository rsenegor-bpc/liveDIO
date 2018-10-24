# set this variable to the RTLinuxPro location
RTLINUXPRO_DIR=/opt/rtldk-2.1
PDROOT = ../../..

include $(RTLINUXPRO_DIR)/rtlinuxpro/rtl.mk

CFLAGS=$(CFLAGS_RTCORE_NAMES) -I$(PDROOT)/include


OBJECT = rtl_int_ao.o

all: $(OBJECT) 

clean:
	rm -f $(OBJECT)
	
include $(RTL_DIR)/Rules.make
