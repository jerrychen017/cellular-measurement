CC=gcc

CFLAGS = -std=c99 -g -c -Wall -pedantic #-D_POSIX_SOURCE -D_GNU_SOURCE
LDFLAGS = -lpthread #-lrt
COMMON = receive_bandwidth.o send_bandwidth.o net_utils.o sendto_dbg.o bandwidth_utils.o controller.o data_generator.o feedbackLogger.o client.o
# when running on rain1 server, add LDFLAGS to echo_server and echo_client

all: server client

server: server.o $(COMMON)
	$(CC) -o server server.o $(COMMON) $(LDFLAGS)

client: client_main.o $(COMMON)
	$(CC) -o client client_main.o $(COMMON) $(LDFLAGS)

%.o:    %.c
	$(CC) $(CFLAGS) $*.c

clean:
	rm *.o server client
