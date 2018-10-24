# set this variable to the RTLinuxPro location
RTLINUXPRO_DIR=/opt/rtldk-2.2
PDROOT = ../../..

include $(RTLINUXPRO_DIR)/rtlinuxpro/rtl.mk

CFLAGS+= -I$(PDROOT)/include

all: rtl_int_ai.rtl 

clean:
	rm -f *.o *.rtl
	
include $(RTL_DIR)/Rules.make

