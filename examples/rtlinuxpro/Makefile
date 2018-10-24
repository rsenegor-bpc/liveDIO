
CC=gcc
LD=ld

SUBDIRS= pd_rtl_ai pd_rtl_ao pd_rtl_aiao rtl_int_ai rtl_int_ao rtl_DIO_event rtl_simple_ao

all:  $(SUBDIRS) 

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	cd $@ && $(MAKE)
	
	
install:
		
clean:
	for i in $(SUBDIRS); do $(MAKE) clean -C $$i; done; 
	
