
CC=gcc
LD=ld

SUBDIRS=pd_rtl_ai \
	pd_rtl_ao \
	pd_rtl_aiao

all:  $(SUBDIRS) 

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	cd $@ && $(MAKE)
	
	
install:
		
clean:
	for i in $(SUBDIRS); do $(MAKE) clean -C $$i; done; 
	
