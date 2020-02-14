
CC=gcc
CFLAGS=-Wall -pthread -g
LIBS=
OBJS=swap1.o options.o

PROGS= swap1

all: $(PROGS)

%.o : %.c
	$(CC) $(CFLAGS) -c $<

swap1: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f $(PROGS) *.o *~

