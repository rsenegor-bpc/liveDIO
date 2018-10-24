CC=gcc
CCFLAGS= -g -Wall -I../../include 
LDFLAGS= -lpowerdaq32 -lm

target=  FifoFix 
objects= Fifo_Size.o 

all: $(target)

$(target): $(objects)
	$(CC) $(objects) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -f $(objects)
	rm -f $(target)
	

