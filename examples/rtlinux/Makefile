
CC=gcc
LD=ld

SUBDIRS=rtl_buffered_ai rtl_buffered_ao pd_rtl_ai pd_rtl_ao rtl_int_ai rtl_int_ao rtl_DIO_event rtl_UctDsp_event rtl_dma_ai rtl_DIO rtl_scan_int rtl_simple_ao

all:  $(SUBDIRS) 

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	cd $@ && $(MAKE)
	
	
install:
		
clean:
	for i in $(SUBDIRS); do $(MAKE) clean -C $$i; done; 
	
