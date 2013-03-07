OBJS=ccache.o runner.o
CC=gcc
CFLAGS=-O3 -Wall

ccache.exe: $(OBJS)
	$(CC) $(OBJS)

.c.o:
	$(CC) -c $(CFLAGS) $<