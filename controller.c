#include "net_include.h"
/**
Controller 
*/
int main(int argc, char *argv[])
{

    // args error checking
    if (argc != 3)
    {
        printf("controller usage: controller <host_address> <port>\n");
        exit(1);
    }

    // port
    int port = atoi(argv[2]);
    // addres
    char *address = argv[1];

    // TODO: receive data from data_generator
    // TODO: send data to server

    char *out = client_send(address, port);
    printf("%s", out); // print out string
    free(out);
    return 0;
}
