CC=gcc


CFLAGS= -O2 -g -Wall -I../../../include -I ../UctDspLib
LDFLAGS= -lpowerdaq32

TARGET=DspEventll
OBJECTS=../UctDspLib/powerdaq_Uct.o DspEventll.o

all:  $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@ 
	
.c.o:
	$(CC) -c $(CFLAGS) -c $< -o $@
	
clean:
	rm -f $(OBJECTS)
	rm -f $(TARGET)
