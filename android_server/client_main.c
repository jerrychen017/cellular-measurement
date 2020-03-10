#include "client.h"
/**
For easiler depolyment to android app, client_main has a similar setup to native-lib.cpp in android NDK. 
*/ 
int main(int argc, char* argv[]) {

// args error checking
if (argc != 3) {
    printf("echo_client usage: echo_client <host_address> <port>\n");
    exit(1);
}

// port
int port = atoi(argv[2]);
// addres
char * address = argv[1];
char * out = client_send(address, port);
printf("%s", out); // print out string 
free(out);
return 0;
}
