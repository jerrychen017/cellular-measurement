#include "send_bandwidth.h"
#include "receive_bandwidth.h"
#include "net_utils.h"
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
    pthread_t tid; // thread id
    struct sockaddr_in send_addr = addrbyname(address, CLIENT_SEND_PORT);
    // arguments to be passed in the new thread
    struct send_bandwidth_args send_args; 
    send_args.addr = send_addr; 
    send_args.port = CLIENT_SEND_PORT;
    pthread_create(&tid, NULL, &send_bandwidth, (void *)&send_args);

    // bind socket
    int sk = setup_bound_socket(CLIENT_RECEIVE_PORT);
    receive_bandwidth(sk, pred_mode);
    return 0;
}
