CC=gcc

CFLAGS = -g -c -Wall -pedantic

all: echo_server echo_client  

echo_server: echo_server.o
	    $(CC) -o echo_server echo_server.o 

echo_client: echo_client.o
		$(CC) -o echo_client echo_client.o

echo_server.o: echo_server.c 
	    $(CC) $(CFLAGS) echo_server.c 

echo_client.o: echo_client.c
	    $(CC) $(CFLAGS) echo_client.c

clean:
	rm *.o
	rm echo_server
	rm echo_client
