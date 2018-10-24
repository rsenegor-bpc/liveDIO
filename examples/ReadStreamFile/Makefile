CC=gcc
CCFLAGS= -g -Wall -I../../include  -I../gnuplot 
LDFLAGS= 

target= ReadStreamFile
OBJECTS= ReadStreamFile.o ../gnuplot/gnuplot.o

all: $(target)

$(target): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@ 

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS)
	rm -f $(target)
