CC=gcc
CCFLAGS= -g -Wall -I../../include 
LDFLAGS= -lpowerdaq32 -lm

target= SingleAO_SimUpdate
OBJECTS= SingleAO_SimUpdate.o 

all: $(target)

$(target): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@ 

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS)
	rm -f $(target)
