# Use gcc
CC=gcc

# Flags:
#   -Wall - Show all warnings
CFLAGS=-c -Wall

all: canbusd

canbusd: canbusd.o canhandle.o fifo-ipc.o
	$(CC) -lcanlib canbusd.o canhandle.o fifo-ipc.o -o canbusd

canbusd.o: canbusd.c
	$(CC) $(CFLAGS) -lcanlib canbusd.c

canhandle.o: canhandle.c
	$(CC) $(CFLAGS) -lcanlib canhandle.c

fifo-ipc.o: fifo-ipc.c
	$(CC) $(CFLAGS) fifo-ipc.c

clean:
	rm -rf *o canbusd
