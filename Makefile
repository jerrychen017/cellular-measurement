CC=gcc

CFLAGS = -g -c -Wall -pedantic

all: echo_server echo_client  

echo_server: echo_server.o utils.o
	    $(CC) -o echo_server echo_server.o utils.o

echo_client: echo_client.o utils.o
		$(CC) -o echo_client echo_client.o utils.o

echo_server.o: echo_server.c include/net_include.h utils.h
	    $(CC) $(CFLAGS) echo_server.c 

echo_client.o: echo_client.c include/net_include.h utils.h
	    $(CC) $(CFLAGS) echo_client.c

utils.o: utils.c utils.h include/net_include.h
		$(CC) $(CFLAGS) utils.c

clean:
	rm *.o
	rm echo_server
	rm echo_client
