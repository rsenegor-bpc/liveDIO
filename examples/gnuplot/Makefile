CC=gcc
CCFLAGS= -g -Wall 
LDFLAGS= -lm -lpthread

target= testgnuplot
OBJECTS= testgnuplot.o gnuplot.o

all: $(target)

$(target): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@ 

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS)
	rm -f $(target)
