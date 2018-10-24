# set this variable to the RTLinuxPro location
RTLINUXPRO_DIR=/opt/rtldk-2.2
PDROOT = ../../..

include $(RTLINUXPRO_DIR)/rtlinuxpro/rtl.mk

CFLAGS+=$(CFLAGS_RTCORE_NAMES) -I$(PDROOT)/include


all: pd_rtl_ao.rtl 

clean:
	rm -f $(OBJECT)
	
include $(RTL_DIR)/Rules.make
