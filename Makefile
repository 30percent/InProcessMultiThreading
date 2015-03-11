CC = gcc
CFLAGS = -Wall -c -g

OFILES = mythreads.o list.o
HEADERS = list.h mythreads.h

all:  libmythreads.a

libmythreads.a: mythreads.o list.o
	ar -cvr $@ $(OFILES)
	
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $<

clean:
	rm -rf *.o *.a