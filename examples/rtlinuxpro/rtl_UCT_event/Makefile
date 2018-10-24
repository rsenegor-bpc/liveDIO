# set this variable to the RTLinuxPro location
RTLINUXPRO_DIR=/opt/rtldk-2.2
PDROOT = ../../..

include $(RTLINUXPRO_DIR)/rtlinuxpro/rtl.mk

CFLAGS := $(filter-out -msoft-float,$(CFLAGS)) -mhard-float
CFLAGS += -I$(PDROOT)/include

all: rtl_UCT_event.o rtl_UCT_event.rtl 

rtl_UCT_event.o: uct_event.o uct8254.o
	$(LD) -r -o rtl_UCT_event.o $?

clean:
	rm -f *.o *.rtl
	
include $(RTL_DIR)/Rules.make
