ifeq ($(XENO_DIR),)
    XENO_DIR = /usr/xenomai
endif

XENO_LIB_DIR = $(shell $(XENO_DIR)/bin/xeno-config --library-dir)

CC     = $(shell $(XENO_DIR)/bin/xeno-config --cc)
CFLAGS = -g $(shell $(XENO_DIR)/bin/xeno-config --xeno-cflags) -I../../../include -I../../gnuplot -I../../ParseParams
LDFLAGS= -lpowerdaq32 -lpthread -lnative -lrtdm -L $(XENO_LIB_DIR)

target= SingleAI
OBJECTS= SingleAI.o ../../gnuplot/gnuplot.o ../../ParseParams/ParseParams.o

all: $(target)

$(target): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@ 

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS)
	rm -f $(target)
