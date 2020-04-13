CC=gcc

CFLAGS = -std=c99 -g -c -Wall -pedantic #-D_POSIX_SOURCE -D_GNU_SOURCE
LDFLAGS = -lpthread -lrt
# when running on rain1 server, add LDFLAGS to echo_server and echo_client

all: controller data_generator server 

server: server.o bandwidth_utils.o sendto_dbg.o
	$(CC) -o bandwidth_server server.o bandwidth_utils.o sendto_dbg.o

controller: controller_main.o controller.o bandwidth_utils.o feedbackLogger.o sendto_dbg.o
	$(CC) -o controller controller_main.o controller.o bandwidth_utils.o feedbackLogger.o sendto_dbg.o

data_generator: data_generator_main.o data_generator.o bandwidth_utils.o
	$(CC) -o data_generator data_generator_main.o data_generator.o bandwidth_utils.o

%.o:    %.c
	$(CC) $(CFLAGS) $*.c

clean:
	rm *.o bandwidth_server controller data_generator
