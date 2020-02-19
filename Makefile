CC=gcc

CFLAGS = -g -c -Wall -pedantic

all: client_main echo_server

echo_server: echo_server.o utils.o
	    $(CC) -o echo_server echo_server.o utils.o

client_main: client_main.o utils.o echo_client.o
	    $(CC) -o client_main echo_client.o utils.o client_main.o

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
	rm echo_client
	rm client_main
