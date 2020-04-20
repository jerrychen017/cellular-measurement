#include "send_bandwidth.h"
/**
    CLI Client 
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
    // address
    char *address = argv[1];
    int ret = start_controller(address, port, false); 
    return ret;
}