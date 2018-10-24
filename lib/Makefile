CC=gcc
LD=gcc
VERSION_MAJOR=1
VERSION_MINOR=0
VERSION_EXTRA=0

prefix=/usr/local
libname=libpowerdaq32.so

XENOMAI_DIR=/usr/xenomai
			  
CFLAGS= -O2 -fPIC -Wall -DPD_VERSION_MAJOR=$(VERSION_MAJOR) -DPD_VERSION_MINOR=$(VERSION_MINOR) -DPD_VERSION_EXTRA=$(VERSION_EXTRA)
LDFLAGS= -shared -Wl,-soname,$(libname).$(VERSION_MAJOR)
DEBUGFLAGS=-DPD_DEBUG -g

ifeq ($(DEBUG),1)
	CFLAGS += ${DEBUGFLAGS}
endif
ifeq ($(XENOMAI),1)
	CFLAGS += -D_PD_XENOMAI -I $(XENOMAI_DIR)/include
	LDFLAGS += -lrtdm -L $(XENOMAI_DIR)/lib
endif
ifeq ($(MEASLAT),1)
	CFLAGS += -DMEASURE_LATENCY
endif


TARGET=$(libname).$(VERSION_MAJOR).$(VERSION_MINOR)
OBJECTS=powerdaq32.o pd_hcaps.o pwrdaqct.o pwrdaqes.o pxi.o

all:  $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ -lc $^

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<
	
install:
	install $(TARGET) $(prefix)/lib
	(cd $(prefix)/lib; ln -sf $(TARGET) $(libname).$(VERSION_MAJOR))
	(cd $(prefix)/lib; ln -sf $(libname).$(VERSION_MAJOR) $(libname))
	if ! grep $(prefix)/lib /etc/ld.so.conf > /dev/null; then \
	   echo "$(prefix)/lib" >> /etc/ld.so.conf; \
	fi
	
clean:
	rm -f *.o
	rm -f $(TARGET)
	

