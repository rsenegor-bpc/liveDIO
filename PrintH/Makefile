CC=gcc

export CFLAGS=-D__KERNEL__ -DMODULE -O2 -Wall -I/lib/modules/$(shell uname -r)/build/include -fno-strict-aliasing


:  PrintH.o
	$(LD) -r $^ -o $@
PrintH.o: PrintH.c PrintH.h

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f PrintH.o
	clear



