CC=gcc
CCFLAGS= -g -Wall -I../../include -I../gnuplot -I../ParseParams
LDFLAGS= -lpowerdaq32 -lpthread -lm -lpthread -lrt

target= BufferedAIAO_Replay
OBJECTS= BufferedAIAO_Replay.o ../gnuplot/gnuplot.o ../ParseParams/ParseParams.o

all: $(target)

$(target): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@ 

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS)
	rm -f $(target)
