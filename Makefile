OBJS=ccache.o ccache_example.o
CC=gcc
CFLAGS=-O3 -Wall -Wno-unused-function

ccache.exe: $(OBJS)
	$(CC) $(OBJS)

.c.o:
	$(CC) -c $(CFLAGS) $<

clean:
	\rm *.o