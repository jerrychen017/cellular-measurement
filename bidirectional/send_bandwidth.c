#include "send_bandwidth.h"
#include "controller.h"
#include "data_generator.h"
#include "net_utils.h"

void * send_bandwidth(void * args) {
    struct send_bandwidth_args * recv_args = (struct send_bandwidth_args *) args;
    int ret;
    pthread_t tid; // thread id for data generator
    struct data_generator_args send_args;

    // port
    int port = recv_args->port;
    printf("send_bandwidth called with port %d\n", port);
    int sk = setup_bound_socket(port);
    // address
    struct sockaddr_in addr = recv_args->addr;

    send_args.android = recv_args->android;
    pthread_create(&tid, NULL, &start_generator_pthread, (void *)&send_args);

    ret = start_controller(recv_args->android, addr, sk); 
    
    return NULL; 
} 
