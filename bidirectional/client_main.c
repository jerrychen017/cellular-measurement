#include "client.h"
#include <stdbool.h>
/**
    CLI Client 
*/
int main(int argc, char *argv[])
{
    // args error checking
    if (argc != 3)
    {
        printf("client usage: client <server_address> <EWMA/RunningAvg>\n");
        exit(1);
    }
    // address
    char *address = argv[1];
    // prediction mode 
    int pred_mode = atoi(argv[2]); 

    start_client(address, pred_mode, false);    
    return 0;
}
