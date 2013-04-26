OBJS=ccache.o ccache_server.o circBuffer.o creqlinkedlist.o 
CFLAGS=-g -I. -Wall -Wextra -pthread -Wno-unused-function
BIN=ccache
CC=gcc

%.o:%.c
	$(CC) $(CFLAGS) $(DEFINES) -o $@ -c $<

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(DEFINES) -o $(BIN) $^

clean:
	rm $(BIN) $(OBJS)