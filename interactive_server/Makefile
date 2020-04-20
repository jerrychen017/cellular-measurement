CC=gcc

CFLAGS = -std=c99 -g -c -Wall -pedantic -D_POSIX_SOURCE -D_GNU_SOURCE
LDFLAGS = -lpthread -lrt
# when running on rain1 server, add LDFLAGS to echo_server and echo_client

all: interactive_server

interactive_server: interactive_server.o 
	$(CC) -o interactive_server interactive_server.o  

interactive_server.o: interactive_server.c net_include.h
	$(CC) $(CFLAGS) interactive_server.c

clean:
	rm *.o
	rm interactive_server