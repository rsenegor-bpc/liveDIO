CC=gcc
LD=ld

SUBDIRS=Event Counter SinglePulse PulseTrain MeasPulseWidth MeasPeriod

all:  $(SUBDIRS) 

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	cd $@ && $(MAKE)
	
	
install:
		
clean:
	for i in $(SUBDIRS); do $(MAKE) clean -C $$i; done; 
