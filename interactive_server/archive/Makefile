CC=gcc

CFLAGS = -std=c99 -g -c -Wall -pedantic -D_POSIX_SOURCE -D_GNU_SOURCE
LDFLAGS = -lpthread -lrt
# when running on rain1 server, add LDFLAGS to echo_server and echo_client

all: client_main echo_server

echo_server: echo_server.o utils.o
	$(CC) $(LDFLAGS) -o echo_server echo_server.o utils.o 

client_main: client_main.o utils.o echo_client.o
	$(CC) $(LDFLAGS) -o client_main echo_client.o utils.o client_main.o

client_main.o: client_main.c include/net_include.h echo_client.h
	$(CC) $(CFLAGS) client_main.c

echo_server.o: echo_server.c include/net_include.h utils.h
	$(CC) $(CFLAGS) echo_server.c 

echo_client.o: echo_client.c include/net_include.h utils.h echo_client.h
	$(CC) $(CFLAGS) echo_client.c

utils.o: utils.c utils.h include/net_include.h
	$(CC) $(CFLAGS) utils.c

clean:
	rm *.o
	rm echo_server
	rm client_main
