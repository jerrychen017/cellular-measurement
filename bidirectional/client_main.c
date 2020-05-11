#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/**
 * CLI Client program
 */
int main(int argc, char *argv[])
{
    // args error checking
    if (argc != 7)
    {
        printf("client usage: client <server_address> <client_send_port> <client_recv_port> <EWMA/RunningAvg> <grace_period> <use_tcp>\n");
        exit(1);
    }
    // address
    char *address = argv[1];
    int client_send_port = atoi(argv[2]);
    int client_recv_port = atoi(argv[3]);
    struct parameters params;
    // prediction mode
    int pred_mode = atoi(argv[4]);
    int grace_period = atoi(argv[5]);
    int use_tcp = atoi(argv[6]);

    params.burst_size = 10;
    params.interval_size = 500;
    params.grace_period = grace_period;
    params.instant_burst = 0;
    params.pred_mode = pred_mode;
    params.use_tcp = use_tcp;
    params.alpha = 0.1;
    params.threshold = 0.95;
    params.interval_time = 1;
    params.min_speed = 0.1;
    params.max_speed = 10;
    params.start_speed = 1;

    start_client(address, params, client_send_port, client_recv_port);
    return 0;
}
