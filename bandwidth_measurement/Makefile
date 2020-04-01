CC=gcc

CFLAGS = -std=c99 -g -c -Wall -pedantic #-D_POSIX_SOURCE -D_GNU_SOURCE
LDFLAGS = -lpthread -lrt
# when running on rain1 server, add LDFLAGS to echo_server and echo_client

all: controller data_generator server 

server: server.o utils.o sendto_dbg.o
	$(CC) -o server server.o utils.o sendto_dbg.o

controller: controller.o utils.o sendto_dbg.o
	$(CC) -o controller controller.o utils.o sendto_dbg.o

data_generator: data_generator.o utils.o
	$(CC) -o data_generator data_generator.o utils.o

%.o:    %.c
	$(CC) $(CFLAGS) $*.c

clean:
	rm *.o server controller data_generator
